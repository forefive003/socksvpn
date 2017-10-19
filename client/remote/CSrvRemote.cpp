#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "proxyConfig.h"

#include "CConnection.h"
#include "CConMgr.h"
#include "CClient.h"
#include "CSrvRemote.h"

const char* g_remote_status_desc[] =
{
    "INVALID",
    "INIT",
    "HANDSHAKING",
    "AUTHING",
    "CONNECTING",
    "CONNECTED",
};

int CSrvRemote::connect_handle(BOOL result)
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    if (!result)
    {
        _LOG_WARN("fail to connect to remote %s/%u", m_ipstr, m_port);
        pConn->client_connect_result_handle(false, 0);
        /*先发送报文*/
        pConn->notify_remote_close();
        /*在释放client*/
        pConn->free_client();
        return 0;
    }
    
    //g_total_remote_cnt++;

    if(0 != this->register_read())
    {
        return -1;
    }
    
    set_remote_status(SOCKS_HANDSHAKING);

    proxy_cfg_t* cfginfo = proxy_cfg_get();
    if (cfginfo->proxy_proto == SOCKS_4)
    {
        char handshake_req[9] = {0};
        uint32_t remote_ipaddr = 0;
        uint16_t remote_port = 0;
        pConn->get_client_real_remote_info(&remote_ipaddr, &remote_port);

        handshake_req[0] = 0x04;
        handshake_req[1] = 0x01;

        *((uint16_t*)&handshake_req[2]) = htons(remote_port);
        *((uint32_t*)&handshake_req[4]) = htonl(remote_ipaddr);
        handshake_req[8] = 0x00;

        return this->send_data(handshake_req, sizeof(handshake_req));
    }
    else
    {
        if (cfginfo->username[0] != 0)
        {
            char handshake_req[4] = {0x05, 0x02, 0x00, 0x02};
            return this->send_data(handshake_req, sizeof(handshake_req));
        }
        else
        {
            char handshake_req[3] = {0x05, 0x01, 0x00};
            return this->send_data(handshake_req, sizeof(handshake_req));
        }
    }

    return 0;
}

int CSrvRemote::socks4_handshake_resp_handle(char *buf, int buf_len)
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    if (buf_len < 8)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) handle handshake resp failed", m_ipstr, m_port, m_fd);
        return -1;
    }

    if (buf[0] != 0x00 || buf[1] != 0x5A)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) handshake failed", m_ipstr, m_port, m_fd);
        return -1;
    }

    set_remote_status(SOCKS_CONNECTED);
	///TODO:
    pConn->client_connect_result_handle(true, 0);
    return 0;
}

int CSrvRemote::socks5_handshake_resp_handle(char *buf, int buf_len)
{
    if (buf_len < 2)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) handle handshake resp failed", m_ipstr, m_port, m_fd);
        return -1;
    }

    if (buf[0] != 0x05)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) handshake failed", m_ipstr, m_port, m_fd);
        return -1;
    }

    if (buf[1] == 0x00)
    {
        set_remote_status(SOCKS_CONNECTING);

        /*05 01 00 01 CA 6C 16 05 00 50
        1、05固定
        2、01说明tcp
        3、00固定
        4、01说明是ip地址
        5、CA 6C 16 05就是202.108.22.5了，百度ip
        6、00 50端口，即为80端口*/

        char connect_req[10] = {0};
        uint32_t remote_ipaddr = 0;
        uint16_t remote_port = 0;
        CConnection *pConn = (CConnection*)this->m_owner_conn;
        pConn->get_client_real_remote_info(&remote_ipaddr, &remote_port);

        connect_req[0] = 0x05;
        connect_req[1] = 0x01;
        connect_req[2] = 0x00;
        connect_req[3] = 0x01;
        *((uint32_t*)&connect_req[4]) = htonl(remote_ipaddr);
        *((uint16_t*)&connect_req[8]) = htons(remote_port);

        return this->send_data(connect_req, sizeof(connect_req));
    }
    else if (buf[1] == 0x02)
    {
        set_remote_status(SOCKS_AUTHING);

		proxy_cfg_t* cfginfo = proxy_cfg_get();
        char auth_req[256] = {0};
        int cur_pos = 0 ;

        auth_req[0] = 0x01;
        auth_req[1] = (char)strlen(cfginfo->username);
        memcpy(&auth_req[2], cfginfo->username, auth_req[1]);
        cur_pos = 2+auth_req[1];
		auth_req[cur_pos] = (char)strlen(cfginfo->passwd);
        memcpy(&auth_req[cur_pos+1], cfginfo->passwd, auth_req[cur_pos]);
		cur_pos += 1 + auth_req[cur_pos];

		return this->send_data(auth_req, cur_pos);
    }

    _LOG_ERROR("remote(%s/%u/fd%d) handshake failed, type %d", m_ipstr, m_port, m_fd, buf[1]);
    return -1;
}

int CSrvRemote::socks5_auth_resp_handle(char *buf, int buf_len)
{
    if (buf_len < 2)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) handle auth resp failed", m_ipstr, m_port, m_fd);
        return -1;
    }

    if (buf[0] != 0x01 || buf[1] != 0x00)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) auth failed", m_ipstr, m_port, m_fd);
        return -1;
    }

    set_remote_status(SOCKS_CONNECTING);

    /*05 01 00 01 CA 6C 16 05 00 50
    1、05固定
    2、01说明tcp
    3、00固定
    4、01说明是ip地址
    5、CA 6C 16 05就是202.108.22.5了，百度ip
    6、00 50端口，即为80端口*/

    char connect_req[10] = {0};
    uint32_t remote_ipaddr = 0;
    uint16_t remote_port = 0;
    CConnection *pConn = (CConnection*)this->m_owner_conn;
    pConn->get_client_real_remote_info(&remote_ipaddr, &remote_port);
    
    connect_req[0] = 0x05;
    connect_req[1] = 0x01;
    connect_req[2] = 0x00;
    connect_req[3] = 0x01;
    *((uint32_t*)&connect_req[4]) = htonl(remote_ipaddr);
    *((uint16_t*)&connect_req[8]) = htons(remote_port);

    return this->send_data(connect_req, sizeof(connect_req));
}

int CSrvRemote::socks5_connect_resp_handle(char *buf, int buf_len)
{
    if (buf_len < 10)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) handle connect resp failed", m_ipstr, m_port, m_fd);
        return -1;
    }

    if (buf[0] != 0x05 || buf[1] != 0x00)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) connect failed", m_ipstr, m_port, m_fd);
        return -1;
    }

    set_remote_status(SOCKS_CONNECTED);

    CConnection *pConn = (CConnection*)this->m_owner_conn;
	///TODO
    pConn->client_connect_result_handle(true, 0);
    return 0;
}


int CSrvRemote::recv_handle(char *buf, int buf_len)
{
    int ret = 0;
    CConnection *pConn = (CConnection *)this->m_owner_conn;

    _LOG_DEBUG("recv from remote(%s/%u/fd%d)", m_ipstr, m_port, m_fd);

    proxy_cfg_t* cfginfo = proxy_cfg_get();

    switch(this->m_status)
    {
    case SOCKS_INIT:
        break;
    case SOCKS_HANDSHAKING:
        if (cfginfo->proxy_proto == SOCKS_4)
        {
            if (0 != this->socks4_handshake_resp_handle(buf, buf_len))
            {
                _LOG_ERROR("remote(%s/%u/fd%d), handle socks4 failed", m_ipstr, m_port, m_fd);
                return -1;
            }
        }
        else if (cfginfo->proxy_proto == SOCKS_5)
        {
            if (0 != this->socks5_handshake_resp_handle(buf, buf_len))
            {
                _LOG_ERROR("client(%s/%u/fd%d), handle socks5 failed", m_ipstr, m_port, m_fd);
                return -1;
            }
        }
        break;

	case SOCKS_AUTHING:
        if (0 != this->socks5_auth_resp_handle(buf, buf_len))
        {
            return -1;
        }
        break;

	case SOCKS_CONNECTING:
        if (0 != this->socks5_connect_resp_handle(buf, buf_len))
        {
            return -1;
        }       
        break;

	case SOCKS_CONNECTED:
        /*发送给远程*/
        ret = pConn->fwd_remote_data_msg(buf, buf_len);
        if (0 != ret)
        {
            _LOG_ERROR("remote(%s/%u/fd%d), send to client failed", m_ipstr, m_port, m_fd);
            return -1;
        }
        break;

    default:
        /// can't be here
        _LOG_ERROR("remote(%s/%u/fd%d) status %d, can't be here", m_ipstr, m_port, m_fd, m_status);
        return -1;
        break;
    }

    return 0;
}

void CSrvRemote::set_remote_status(SOCKS_STATUS_E status)
{
    if (status == m_status)
    {
        _LOG_ERROR("remote(%s/%u/fd%d) status already %s", m_ipstr, m_port, 
            m_fd, g_remote_status_desc[m_status]);
        return;
    }

    m_status = status;
    _LOG_INFO("remote(%s/%u/fd%d) set to status %s", m_ipstr, m_port, 
        m_fd, g_remote_status_desc[m_status]);
    return;
}

int CSrvRemote::get_remote_status()
{
    return m_status;
}

#ifndef _WIN32
#include <netinet/tcp.h>
#endif

#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "proxyConfig.h"

#include "CNetAccept.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "CRemoteServer.h"
#include "relay_pkt_def.h"

MUTEX_TYPE m_remote_srv_lock;
CRemoteServer *g_RemoteServ = NULL;

BOOL is_remote_authed()
{
    MUTEX_LOCK(m_remote_srv_lock);
    if (g_RemoteServ && g_RemoteServ->is_authed())
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return TRUE;
    }
    MUTEX_UNLOCK(m_remote_srv_lock);
    return FALSE;
}

BOOL is_remote_connected()
{
    MUTEX_LOCK(m_remote_srv_lock);
    if (g_RemoteServ && g_RemoteServ->is_connected())
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return TRUE;
    }
    MUTEX_UNLOCK(m_remote_srv_lock);
    return FALSE;
}

void CRemoteServer::free_handle()
{
    ///TODO:
    g_ConnMgr->free_all_conn();
    
    /*set to null, re init in timer*/
    MUTEX_LOCK(m_remote_srv_lock);
    delete g_RemoteServ;
    g_RemoteServ = NULL;
    MUTEX_UNLOCK(m_remote_srv_lock);
}

int CRemoteServer::connect_handle(BOOL result)
{
	if (!result)
	{
		return 0;
	}

#if 0
        int keepalive = 1; // 开启keepalive属性
        int keepidle = 10; // 如该连接在10秒内没有任何数据往来,则进行探测
        int keepinterval = 5; // 探测时发包的时间间隔为5 秒
        int keepcount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
        setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
        setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
        setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
        setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));
#endif
	if(0 != this->register_read())
	{
        return -1;
	}
    else
    {
	    this->send_auth_quest_msg();
    }

	return 0;
}


int CRemoteServer::build_authen_msg(char *buf)
{
    int pos = 0;

	proxy_cfg_t *cfginfo = proxy_cfg_get();
    if (cfginfo->is_auth)
    {
        buf[0] = 0x01;
    	buf[1] = strlen(cfginfo->username);
    	strncpy(&buf[2], cfginfo->username, buf[1]);
        pos += buf[1] + 2;

    	buf[pos] = strlen(cfginfo->passwd);
    	strncpy(&buf[pos + 1], cfginfo->passwd, buf[pos]);
        pos += buf[pos] + 1;
    }
    else
    {
        buf[0] = 0x01;
        buf[1] = 0x00;
        buf[2] = 0x00;
        pos = 3;
    }

    return pos;
}

BOOL CRemoteServer::parse_authen_result_msg(char *buf, int buf_len)
{
    if (buf_len < 6)
    {
        _LOG_ERROR("authen failed, buf_len %d too short", buf_len);
        return FALSE;
    }

    if (buf[0] != 0x01)
    {
        _LOG_ERROR("buf invalid when parse authen result, %x", buf[0]);
        return FALSE;
    }

    BOOL result = FALSE;

    if (buf[1] != 0x00)
    {
        //_LOG_ERROR("authen failed, ret %x", buf[1]);
        result = FALSE;
    }
    else
    {
        result  = TRUE;
    }

    int srv_ip_array[32] = {0};
    int *ptmp = (int*)&buf[2];
    int srv_cnt = *ptmp;
    ptmp++;

    srv_cnt = ntohl(srv_cnt);
    for(int ii = 0; ii < srv_cnt; ii++)
    {
        srv_ip_array[ii] = *ptmp;
        ptmp++;

        srv_ip_array[ii] = ntohl(srv_ip_array[ii]);
    }

    proxy_set_servers(srv_ip_array, srv_cnt);
    
    return result;
}

BOOL CRemoteServer::parse_connect_result_msg(char *buf, int buf_len)
{
    if (buf[0] != 0x05 || buf[3] != 0x01)
    {
        _LOG_ERROR("buf invalid when parse connect result, %x", buf[0]);
        return FALSE;
    }

    if ((uint8_t)buf[1] == CONNECT_FAILED_FLAG)
    {
        return FALSE;
    }

    return TRUE;
}

int CRemoteServer::send_auth_quest_msg()
{
    PKT_HDR_T pkthdr;
    PKT_C2R_HDR_T c2rhdr;
    char buf[512] = {0};
    int buf_len = 0;

    buf_len = this->build_authen_msg(buf);

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    pkthdr.pkt_type = htons(PKT_C2R);
    pkthdr.pkt_len = htons(sizeof(PKT_C2R_HDR_T) + buf_len);

    memset(&c2rhdr, 0, sizeof(PKT_C2R_HDR_T));

	proxy_cfg_t *cfginfo = proxy_cfg_get();

    c2rhdr.server_ip = htonl(cfginfo->server_ip);
    c2rhdr.client_ip = htonl(m_local_ipaddr);
    c2rhdr.server_port = 0;
    c2rhdr.client_port = htons(cfginfo->local_port);
    c2rhdr.sub_type = htons(C2R_AUTH);
    c2rhdr.reserved = 0;

    if(0 != this->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        return -1;
    }
    if(0 != this->send_data((char*)&c2rhdr, sizeof(PKT_C2R_HDR_T)))
    {
        return -1;
    }
    if(0 != this->send_data(buf, buf_len))
    {
        return -1;
    }

    _LOG_INFO("send auth request msg to remote");
    return 0;
}


BOOL CRemoteServer::is_authed()
{
	return m_is_authed;
}
int CRemoteServer::auth_result_msg_handle(BOOL result)
{
	if (result)
	{
        _LOG_INFO("auth success");
		m_is_authed = TRUE;
	}
	else
	{
        _LOG_INFO("auth failed");
		m_is_authed = FALSE;
	}
    return 0;
}

int CRemoteServer::pdu_handle(char *pdu_buf, int pdu_len)
{
    int ret = 0;
    PKT_HDR_T *pkthdr = (PKT_HDR_T*)pdu_buf;

    PKT_R2C_HDR_T *r2chdr = (PKT_R2C_HDR_T*)(pdu_buf + sizeof(PKT_HDR_T));
    PKT_R2C_HDR_NTOH(r2chdr);

    if (pkthdr->pkt_type != PKT_R2C)
    {
        _LOG_ERROR("recv pkt type %d invalid", pkthdr->pkt_type);
        _LOG_ERROR("pktlen %d, client:0x%x/0x%x, server:0x%x/0x%x, subtype 0x%x", pkthdr->pkt_len, 
            r2chdr->client_ip,
            r2chdr->client_port,
            r2chdr->server_ip,
            r2chdr->server_port, 
            r2chdr->sub_type);
        return -1;
    }

    char *data_buf = pdu_buf + sizeof(PKT_HDR_T) + sizeof(PKT_R2C_HDR_T);
    int data_len = pkthdr->pkt_len - sizeof(PKT_R2C_HDR_T);

    if (r2chdr->sub_type == R2C_AUTH_RESULT)
    {
        BOOL result = parse_authen_result_msg(data_buf, data_len);
        auth_result_msg_handle(result);
    }
    else
    {
        CConnection *pConn = (CConnection*)g_ConnMgr->get_conn_by_client(r2chdr->client_ip, 
                                    r2chdr->client_port, 0, 0);
        if (NULL == pConn)
        {
            _LOG_WARN("failed to find connection for client 0x%x/%u when recv msg:%d",  
                    r2chdr->client_ip, r2chdr->client_port,
                    r2chdr->sub_type);
            /*can't close connect to remote*/
            return 0;
        }       
        
        if (r2chdr->sub_type == R2C_CONNECT_RESULT)
        {
            if (SOCKS_CONNECTING != pConn->get_client_status())
            {
                _LOG_ERROR("client(0x%x/%u/fd%d) not in connecting when recv connect result", 
                    pConn->get_client_ipaddr(), pConn->get_client_port(), pConn->get_client_fd());
                /*can't close connect to remote*/
                pConn->dec_ref();
                return 0;
            }

            if (FALSE == parse_connect_result_msg(data_buf, data_len))
            {
                /*authen failed*/
                pConn->client_connect_result_handle(FALSE);
            }
            else
            {
                /*authen ok*/
                pConn->client_connect_result_handle(TRUE);
            }
        }
        else if (r2chdr->sub_type == R2C_DATA)
        {
            if (SOCKS_CONNECTED != pConn->get_client_status())
            {
                _LOG_ERROR("client(0x%x/%u/fd%d) not authed yet when recv data", 
                    pConn->get_client_ipaddr(), pConn->get_client_port(), pConn->get_client_fd());
                /*can't close connect to remote*/
                pConn->dec_ref();
                return 0;
            }

            pConn->fwd_remote_data_msg(data_buf, data_len);
        }
        else if (r2chdr->sub_type == REMOTE_CLOSED)
        {
            _LOG_INFO("recv remote closed msg on client(0x%x/%u/0x%x/%u/fd %d)",
                pConn->get_client_ipaddr(), pConn->get_client_port(),
                pConn->get_client_inner_ipaddr(), pConn->get_client_inner_port(),
                pConn->get_client_fd());
            pConn->m_remote->free();
        }
        else
        {
            _LOG_ERROR("client(0x%x/%u/fd %d) recv invalid data type %d in r2chdr", r2chdr->sub_type, 
                    pConn->get_client_ipaddr(), pConn->get_client_port(), pConn->get_client_fd());
            pConn->dec_ref();
            return -1;
        }   
        pConn->dec_ref();
    }

    return ret;
}

int CRemoteServer::recv_handle(char *buf, int buf_len)
{
    int ret = 0;
    while (true)
    {
        if (buf_len > 0)
        {
            /*it has data not copy yet, copy it now*/
            int spare_len = sizeof(m_recv_buf) - m_recv_len;
            if (buf_len > spare_len)
            {
                memcpy(&m_recv_buf[m_recv_len], buf, spare_len);
                m_recv_len = sizeof(m_recv_buf);

                buf_len -= spare_len;
                buf += spare_len;
            }
            else
            {
                memcpy(&m_recv_buf[m_recv_len], buf, buf_len);
                m_recv_len += buf_len;

                buf_len = 0;
            }
        }

        if (m_recv_len < (int)(sizeof(PKT_HDR_T)))
        {
            _LOG_DEBUG("cur recv %d, wait for hdr", m_recv_len);
            break;
        }
        PKT_HDR_T *pkthdr = (PKT_HDR_T*)m_recv_buf;
        PKT_HDR_NTOH(pkthdr);
        if (pkthdr->pkt_len > (sizeof(m_recv_buf) - sizeof(PKT_HDR_T)))
        {
            _LOG_ERROR("cur recv %d, but pktlen %u too long", m_recv_len, pkthdr->pkt_len);
            return -1;
        }

        if (m_recv_len < (int)(sizeof(PKT_HDR_T) + pkthdr->pkt_len))
        {
            _LOG_DEBUG("cur recv %d, need %d, wait for data, fd %d", 
                m_recv_len, (sizeof(PKT_HDR_T) + pkthdr->pkt_len), m_fd);
            PKT_HDR_HTON(pkthdr);
            break;
        }

        int data_len = sizeof(PKT_HDR_T) + pkthdr->pkt_len;
        ret = pdu_handle(m_recv_buf, data_len);

        /*refill buffer*/        
        if (m_recv_len > data_len)
        {
            /*memcpy在64位ubuntu上有问题*/
            memmove(m_recv_buf, m_recv_buf + data_len, m_recv_len - data_len);
            m_recv_len -= data_len;
        }
        else
        {
            m_recv_len = 0;
            break;
        }
    }

    ret = ret;
    return 0;
}

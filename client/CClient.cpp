#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CClient.h"
#include "CRemote.h"
#include "CRemoteServer.h"
#include "CSocksMem.h"
#include "socks_client.h"


int CClient::send_data_msg(char *buf, int buf_len)
{
	return send_data(buf, buf_len);
}

int CClient::parse_handshake(char *buf, int buf_len)
{
/*
05 01 00 共3字节，这种是要求匿名代理
05 01 02 共3字节，这种是要求以用户名密码方式验证代理
05 02 00 02 共4字节，这种是要求以匿名或者用户名密码方式代理
*/
	if (buf[0] != 0x05 || buf_len < 3)
	{
		_LOG_ERROR("buf 0x%02x invalid, len %d, handshake failed.", buf[0], buf_len);
		return -1;
	}

	int len = buf[1];
	if ((len + 1) > buf_len)
	{
		_LOG_ERROR("buf too less, handshake failed");
		return -1;
	}

	for (int i = 0; i < len; i++)
	{
		if (buf[i+2] == 0)
		{
			_LOG_INFO("client want anonimous proxy");
			return 0;
		}
		else if (buf[i+2] == 2)
		{
			_LOG_WARN("client want user-password proxy, ignore it");
			return -1;
		}
		else
		{
			_LOG_WARN("client want proxy method %02x, ignore it", buf[i+2]);
			return -1;
		}
	}

	return 0;
}


int CClient::parse_connect(char *buf, int buf_len)
{
	/**
    第一种
        05 01 00 03 13 77  65 62 2E 73 6F 75 72 63  65 66 6F 72 67 65 2E 6E  65 74 00 16
        1、05固定
        2、01说明是tcp
        3、00固定
        4、03说明后面跟着的是域名而不是ip地址，由socks5服务器进行dns解析
        5、13前面指明了是域名，那么0x13（19字节）是域名字符长度
        6、77 65 62 2E 73 6F 75 72 63 65 66 6F 72 67 65 2E 6E 65 74 就这19个是域名web.sourceforge.net的ascii。
        7、00 16端口，即为22端口。
    第二种
        05 01 00 01 CA 6C 16 05 00 50
        1、05固定
        2、01说明tcp
        3、00固定
        4、01说明是ip地址
        5、CA 6C 16 05就是202.108.22.5了，百度ip
        6、00 50端口，即为80端口
    */

	if (buf[0] != 0x05 || buf[2] != 0)
    {
        _LOG_ERROR("data buf invalid.");
        return -1;
    }
    if (buf[1] != 0x01) 
    {
        _LOG_ERROR("buf[1] %d, only support CONNECT CMD now -_-", buf[1]);
        return -1;
    }

    if (buf_len < 4)
    {
        _LOG_ERROR("buf_len too less %d", buf_len);
        return -1;
    }

    uint32_t remote_ipaddr = 0;
    char remote_domain[HOST_DOMAIN_LEN + 1] = {0};
    uint16_t remote_dstport = 0;

    switch (buf[3])
    { /* ATYP */
    case 0x01: /* IPv4 */
        if (buf_len < 10)
        {
            _LOG_ERROR("buf_len too less %d", buf_len);
            return -1;
        }

        memcpy(&remote_ipaddr, &buf[4], 4);
        remote_ipaddr = ntohl(remote_ipaddr);
        memcpy(&remote_dstport, &buf[8], 2);
        remote_dstport = ntohs(remote_dstport);
        _LOG_INFO("connect request ipaddr 0x%x, port %u", remote_ipaddr, remote_dstport);
        break;
    case 0x03: /* FQDN */
        if (buf_len < (5 + buf[4] + 2))
        {
            _LOG_ERROR("buf_len too less %d", buf_len);
            return -1;
        }

        if (buf[4] > HOST_DOMAIN_LEN)
        {
            _LOG_ERROR("domain name too long %d", buf[4]);
            return -1;
        }
        memcpy(remote_domain, &buf[5], buf[4]);

        memcpy(&remote_dstport, &buf[5 + buf[4]], 2);
        remote_dstport = ntohs(remote_dstport);
        _LOG_INFO("connect request domain %s, port %u", remote_domain, remote_dstport);
        break;
    case 0x04: /* IPv6 */
        ///TODO:
        _LOG_WARN("IPV6 not support now");
        return -1;
        break;
    default:
        _LOG_ERROR("err ATYP: %x", buf[3]);
        return -1;
        break;
    }

    return 0;
}

int CClient::recv_handle(char *buf, int buf_len)
{
	int ret = 0;
	CConnection *pConn = (CConnection *)this->m_owner_conn;
	CRemote *pRemote = NULL;

	_LOG_DEBUG("recv from client(%s/%u/fd%d)", m_ipstr, m_port, m_fd);

	switch(this->m_status)
	{
	case CLI_INIT:
		if (0 != this->parse_handshake(buf, buf_len))
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), parse handshake msg failed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}

		this->set_client_status(CLI_AUTHING);

		/*添加一个远程连接*/
		if (is_remote_connected())
		{
			pRemote = new CRemote(g_server_ip, 0, -1, pConn);
			pRemote->init_async_write_resource(socks_malloc, socks_free);
			g_total_remote_cnt++;
            pConn->attach_remote(pRemote);

			if (0 != pRemote->init())
			{
				_LOG_ERROR("client(%s/%u/%s/%u/fd%d), remote init failed", m_ipstr, m_port, 
					m_inner_ipstr, m_inner_port, m_fd);
				pConn->detach_remote();
			    delete pRemote;
			    return -1;
			}
			
			this->auth_result_handle(TRUE);
		}
		else
		{
			_LOG_ERROR("clost client(%s/%u/%s/%u/fd%d), for remote not authed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			this->auth_result_handle(FALSE);
		}
		break;

	case CLI_CONNECTING:
		if (0 != this->parse_connect(buf, buf_len))
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), parse connect msg failed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}

		g_total_connect_req_cnt++;
		this->m_request_time = util_get_cur_time();
		ret = pConn->fwd_client_connect_msg(buf, buf_len);
		if (0 != ret)
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), send connect to remote failed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}		
		break;

	case CLI_CONNECTED:
		/*发送给远程*/
		ret = pConn->fwd_client_data_msg(buf, buf_len);
		if (0 != ret)
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), send to remote failed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}
		break;

	case CLI_AUTHING:
	default:
		/// can't be here
		_LOG_ERROR("client(%s/%u/%s/%u/fd%d) status %d, can't be here", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd, m_status);
		return -1;
		break;
	}

	return 0;
}

int CClient::get_client_status()
{
	return m_status;
}

void CClient::set_client_status(CLI_STATUS_E status)
{
	if (status == m_status)
	{
		_LOG_ERROR("client(%s/%u/%s/%u/fd%d) status already %s", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port,
			m_fd, g_cli_status_desc[m_status]);
		return;
	}

	m_status = status;
	_LOG_INFO("client(%s/%u/%s/%u/fd%d) set to status %s", m_ipstr, m_port, 
		m_inner_ipstr, m_inner_port,
		m_fd, g_cli_status_desc[m_status]);
}

void CClient::auth_result_handle(BOOL result)
{
	if (CLI_AUTHING != m_status)
	{
		_LOG_WARN("client(%s/%u/%s/%u/fd%d) not authing when recv auth result", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port, m_fd);
		return;
	}

	if (result)
	{
        //_LOG_INFO("client(0x%s/%u/fd%d) recv auth success", m_ipstr, m_port, m_fd);
		this->set_client_status(CLI_CONNECTING);

		char buf[2] = {0};
		buf[0] = 0x05;
		buf[1] = 0x00; /* NO AUTHENTICATION REQUIRED */
		if (0 != this->send_data_msg(buf, 2))
		{
			_LOG_ERROR("send failed, handshake failed.");
			this->free();
		}
	}
	else
	{
		_LOG_WARN("client(%s/%u/%s/%u/fd%d) recv auth failed", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port, m_fd);
		this->free();
	}
}

void CClient::connect_result_handle(BOOL result)
{
    if (CLI_CONNECTING != m_status)
    {
        _LOG_WARN("client(%s/%u/%s/%u/fd%d) not connecting when recv connect result", m_ipstr, m_port, 
        	m_inner_ipstr, m_inner_port, m_fd);
        return;
    }

    uint64_t response_time = util_get_cur_time();
    uint64_t consume_time = response_time - this->m_request_time;
    g_total_connect_resp_consume_time += consume_time;
	g_total_connect_resp_cnt++;

    if (result)
    {
        _LOG_INFO("client(%s/%u/%s/%u/fd%d) recv connect success, consume %"PRIu64"s", m_ipstr, m_port, 
        	m_inner_ipstr, m_inner_port, m_fd, consume_time);
        this->set_client_status(CLI_CONNECTED);
    }
    else
    {
        _LOG_WARN("client(%s/%u/%s/%u/fd%d) recv connect failed, consume %"PRIu64"s", m_ipstr, m_port, 
        	m_inner_ipstr, m_inner_port, m_fd, consume_time);
        this->free();
    }
}

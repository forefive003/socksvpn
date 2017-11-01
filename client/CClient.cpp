#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "proxyConfig.h"

#include "CConnection.h"
#include "CConMgr.h"
#include "CClient.h"
#include "CSrvRemote.h"
#include "CVpnRemote.h"
#include "CSocksMem.h"
#include "CRemoteServer.h"
#include "CSyslogMgr.h"
#include "CRemoteServerPool.h"

int CClient::register_req_handle(char *buf, int buf_len)
{
	register_req_t  *reg_req = (register_req_t*)buf;

	if (buf_len < (int)sizeof(register_req_t))
	{
		_LOG_ERROR("client(%s/%u/fd%d) handle register failed", m_ipstr, m_port, m_fd);
		return -1;
	}

	if (ntohl(reg_req->magic) != PROXY_MSG_MAGIC)
	{
		_LOG_ERROR("client(%s/%u/fd%d) recv invalid register, magic should 0x%x, but 0x%x", m_ipstr, m_port, m_fd,
			PROXY_MSG_MAGIC, ntohl(reg_req->magic));
		return -1;
	}

	reg_req->remote_ipaddr = ntohl(reg_req->remote_ipaddr);
	reg_req->remote_port = ntohs(reg_req->remote_port);
	_LOG_INFO("client(%s/%u/fd%d) get register req, remote 0x%x/%d", m_ipstr, m_port, m_fd,
		reg_req->remote_ipaddr, reg_req->remote_port);

	this->m_proc_id = reg_req->proc_id;
	this->set_client_status(SOCKS_CONNECTING);
	this->set_real_server(reg_req->remote_ipaddr, reg_req->remote_port);

	proxy_cfg_t* cfginfo = proxy_cfg_get();
	CConnection *pConn = (CConnection *)this->m_owner_conn;

    CSrvRemote *pRemote = new CSrvRemote(cfginfo->server_ip, 0, -1, pConn);
//    pRemote->init_async_write_resource(socks_malloc, socks_free);    
    pConn->attach_remote(pRemote);
    if (0 != pRemote->init())
    {
        pConn->detach_remote();
        delete pRemote;
        this->set_client_status(SOCKS_CONNECTED_FAILED);
        return -1;
    }

    return 0;
}

int CClient::socks_parse_handshake(char *buf, int buf_len)
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

int CClient::socks_parse_connect(char *buf, int buf_len)
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
        _LOG_ERROR("data buf invalid, not socks5.");
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
        _LOG_INFO("get socks5 req, connect request ipaddr 0x%x, port %u", remote_ipaddr, remote_dstport);

        this->set_real_server(remote_ipaddr, remote_dstport);
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

        this->set_real_server(0, remote_dstport);
        this->set_real_domain(remote_domain);
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

int CClient::socks_proto5_handle(char *buf, int buf_len)
{
	if (0 != this->socks_parse_handshake(buf, buf_len))
	{
		_LOG_ERROR("client(%s/%u/%s/%u/fd%d), parse handshake msg failed", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port, m_fd);
		return -1;
	}

	this->set_client_status(SOCKS_AUTHING);
	this->m_socks_proto_version = 5;

	/*添加一个远程连接*/
	int active_srv_index = g_remoteSrvPool->get_active_conn_obj();
	if (-1 != active_srv_index)
	{
		proxy_cfg_t* cfginfo = proxy_cfg_get();
		CConnection *pConn = (CConnection *)this->m_owner_conn;
		CVpnRemote *pRemote = new CVpnRemote(cfginfo->server_ip, 0, -1, pConn, active_srv_index);
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
		_LOG_WARN("clost client(%s/%u/%s/%u/fd%d), for no authed remote server", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port, m_fd);
		this->auth_result_handle(FALSE);
	}
	return 0;
}

int CClient::socks_proto4_handle(char *buf, int buf_len)
{
	/*04 01 port ipaddr*/
	if (buf_len < 8)
	{
		_LOG_INFO("get socks4 req, but buf len %d too short", buf_len);
		return -1;
	}

	uint32_t remote_ipaddr = 0;
	uint16_t remote_dstport = 0;

	/*2-3为端口*/
	memcpy(&remote_dstport, &buf[2], 2);
    remote_dstport = ntohs(remote_dstport);
    /*4-7为ip地址*/
    memcpy(&remote_ipaddr, &buf[4], 4);
    remote_ipaddr = ntohl(remote_ipaddr);
    
    this->set_real_server(remote_ipaddr, remote_dstport);
    _LOG_INFO("get socks4 req, connect request ipaddr 0x%x, port %u", remote_ipaddr, remote_dstport);

	this->set_client_status(SOCKS_AUTHING);
	this->m_socks_proto_version = 4;

	/*添加一个远程连接*/
	int active_srv_index = g_remoteSrvPool->get_active_conn_obj();
	if (-1 != active_srv_index)
	{
		proxy_cfg_t* cfginfo = proxy_cfg_get();
		CConnection *pConn = (CConnection *)this->m_owner_conn;
		CVpnRemote *pRemote = new CVpnRemote(cfginfo->server_ip, 0, -1, pConn, active_srv_index);
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

		/*05 01 00 01 CA 6C 16 05 00 50
        1、05固定
        2、01说明tcp
        3、00固定
        4、01说明是ip地址
        5、CA 6C 16 05就是202.108.22.5了，百度ip
        6、00 50端口，即为80端口*/
		remote_ipaddr = this->m_real_remote_ipaddr;
		remote_dstport = this->m_real_remote_port;		
		remote_ipaddr = htonl(remote_ipaddr);
		remote_dstport = htons(remote_dstport);

		/*00 5A port ipaddr*/
		char buf[10] = {0};
		buf[0] = 0x05;
		buf[1] = 0x01;
		buf[2] = 0x00;
		buf[3] = 0x01;
		memcpy(&buf[4], &remote_ipaddr, 4);
		memcpy(&buf[8], &remote_dstport, 2);		

		//g_total_connect_req_cnt++;
		this->m_request_time = util_get_cur_time();
		if (0 != pConn->fwd_client_connect_msg(buf, 10))
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), send connect to remote failed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}
	}
	else
	{
		_LOG_WARN("clost client(%s/%u/%s/%u/fd%d), for no authed remote server", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port, m_fd);
		this->auth_result_handle(FALSE);
	}
	return 0;
}

int CClient::recv_handle(char *buf, int buf_len)
{
	int ret = 0;
	CConnection *pConn = (CConnection *)this->m_owner_conn;

	_LOG_DEBUG("recv from client(%s/%u/fd%d)", m_ipstr, m_port, m_fd);

	switch(this->m_status)
	{
	case SOCKS_INIT:
		if (buf_len <= 2)
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), recv data failed in accepcted status", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}
		if ((buf[0] == 0x04 && buf[1] == 0x01)
			|| (buf[0] == 0x05 && buf[1] == 0x01)
			|| (buf[0] == 0x05 && buf[1] == 0x02))
		{
			m_is_standard_socks = true;
			
			/*recv socks packet directly, maybe from chrome or ie*/
			if (!proxy_cfg_is_vpn_type())
			{
				_LOG_ERROR("client(%s/%u/%s/%u/fd%d) recv socks auth req, but not VPN type, ignore", m_ipstr, m_port, 
					m_inner_ipstr, m_inner_port, m_fd);
				return -1;
			}

			if (buf[0] == 0x04)
			{
				return this->socks_proto4_handle(buf, buf_len);
			}

			return this->socks_proto5_handle(buf, buf_len);
		}
		else
		{
			return this->register_req_handle(buf, buf_len);
		}
		break;

	case SOCKS_CONNECTING:
		if (!proxy_cfg_is_vpn_type())
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d) recv socks connect req, but not VPN type, ignore", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}

		if (0 != this->socks_parse_connect(buf, buf_len))
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), parse connect msg failed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}

		//g_total_connect_req_cnt++;
		this->m_request_time = util_get_cur_time();
		ret = pConn->fwd_client_connect_msg(buf, buf_len);
		if (0 != ret)
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), send connect to remote failed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}		
		break;

	case SOCKS_CONNECTED:
		/*发送给远程*/
		ret = pConn->fwd_client_data_msg(buf, buf_len);
		if (0 != ret)
		{
			_LOG_ERROR("client(%s/%u/%s/%u/fd%d), send to remote failed", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd);
			return -1;
		}
		break;

	case SOCKS_AUTHING:
	default:
		/// can't be here
		_LOG_ERROR("client(%s/%u/%s/%u/fd%d) status %d, can't be here", m_ipstr, m_port, 
				m_inner_ipstr, m_inner_port, m_fd, m_status);
		return -1;
		break;
	}
	return 0;
}

void CClient::auth_result_handle(BOOL result)
{
	if (SOCKS_AUTHING != m_status)
	{
		_LOG_WARN("client(%s/%u/%s/%u/fd%d) not authing when recv auth result", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port, m_fd);
		return;
	}

	if (result)
	{
        //_LOG_INFO("client(0x%s/%u/fd%d) recv auth success", m_ipstr, m_port, m_fd);
		this->set_client_status(SOCKS_CONNECTING);

		if (this->m_socks_proto_version == 5)
		{
			char buf[2] = {0};
			buf[0] = 0x05;
			buf[1] = 0x00; /* NO AUTHENTICATION REQUIRED */
			if (0 != this->send_data_msg(buf, 2))
			{
				_LOG_ERROR("send failed, handshake failed.");
				this->free();
			}
		}
	}
	else
	{
		_LOG_WARN("client(%s/%u/%s/%u/fd%d) recv auth failed", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port, m_fd);
		this->free();
	}
}

void CClient::connect_result_handle(BOOL result, int remote_ipaddr)
{
    if (SOCKS_CONNECTING != m_status)
    {
        _LOG_WARN("client(%s/%u/%s/%u/fd%d) not connecting when recv connect result", m_ipstr, m_port, 
        	m_inner_ipstr, m_inner_port, m_fd);
        return;
    }

    uint64_t response_time = util_get_cur_time();
    uint64_t consume_time = response_time - this->m_request_time;
    //g_total_connect_resp_consume_time += consume_time;
	//g_total_connect_resp_cnt++;

    if (result)
    {
        _LOG_INFO("client(%s/%u/%s/%u/fd%d) connect success, consume %"PRIu64"s", m_ipstr, m_port, 
        	m_inner_ipstr, m_inner_port, m_fd, consume_time);

        m_real_remote_ipaddr = remote_ipaddr;
        this->set_client_status(SOCKS_CONNECTED);

        if (consume_time >= 5)
        {
			g_syslogMgr->add_syslog(L_WARN, "client(%s/%u/fd%d) connect success, consume %"PRIu64"s", m_ipstr, m_port, 
    						m_fd, consume_time);
        }
    }
    else
    {
        _LOG_WARN("client(%s/%u/%s/%u/fd%d) connect failed, consume %"PRIu64"s", m_ipstr, m_port, 
        	m_inner_ipstr, m_inner_port, m_fd, consume_time);
        this->set_client_status(SOCKS_CONNECTED_FAILED);

		g_syslogMgr->add_syslog(L_WARN, "client(%s/%u/fd%d) connect failed, consume %"PRIu64"s", m_ipstr, m_port, 
    				m_fd, consume_time);
    }
}

int CClient::get_client_status()
{
	return m_status;
}

void CClient::set_client_status(SOCKS_STATUS_E status)
{
	if (status == m_status)
	{
		_LOG_ERROR("client(%s/%u/%s/%u/fd%d) status already %s", m_ipstr, m_port, 
			m_inner_ipstr, m_inner_port,
			m_fd, g_socks_status_desc[m_status]);
		return;
	}

	m_status = status;
	_LOG_INFO("client(%s/%u/%s/%u/fd%d) set to status %s", m_ipstr, m_port, 
		m_inner_ipstr, m_inner_port,
		m_fd, g_socks_status_desc[m_status]);

	if (proxy_cfg_is_vpn_type())
	{
		if (m_status == SOCKS_CONNECTED)
		{
			if (this->m_socks_proto_version == 4)
			{
				/*socks4*/
				uint32_t remote_ipaddr = this->m_real_remote_ipaddr;
				uint16_t remote_dstport = this->m_real_remote_port;

				/*2-3为端口*/
			    remote_dstport = htons(remote_dstport);
			    /*4-7为ip地址*/
			    remote_ipaddr = htonl(remote_ipaddr);

				/*00 5A port ipaddr*/
				char resp_buf[8] = { 0 };
				resp_buf[0] = 0x00;
				resp_buf[1] = 0x5A;
				memcpy(&resp_buf[2], &remote_dstport, 2);
				memcpy(&resp_buf[4], &remote_ipaddr, 4);
				if (0 != this->send_data_msg(resp_buf, 8))
				{
					_LOG_ERROR("send failed, handshake failed.");
					this->free();
				}
			}
			else
			{
				/*socks5*/
				char resp_buf[10] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				if (0 != this->send_data_msg(resp_buf, sizeof(resp_buf)))
				{
					_LOG_ERROR("send failed, handshake failed.");
					this->free();
				}
			}
		}
	}
	else
	{
		if (m_status == SOCKS_CONNECTED)
		{
			register_resp_t reg_resp;
			reg_resp.result = true;

			this->send_data((char*)&reg_resp, sizeof(reg_resp));
		}
		else if (m_status == SOCKS_CONNECTED_FAILED)
		{
			register_resp_t reg_resp;
			reg_resp.result = false;

			this->send_data((char*)&reg_resp, sizeof(reg_resp));
		}
	}
}

void CClient::set_real_server(uint32_t real_serv, uint16_t real_port)
{
    m_real_remote_ipaddr = real_serv;
    m_real_remote_port = real_port;
}

void CClient::get_real_server(uint32_t *real_serv, uint16_t *real_port)
{
    *real_serv = m_real_remote_ipaddr;
    *real_port = m_real_remote_port;
}

void CClient::set_real_domain(char *domain_name)
{
	memset(m_remote_domain, 0, HOST_DOMAIN_LEN + 1);
	strncpy(m_remote_domain, domain_name, HOST_DOMAIN_LEN);
}

void CClient::get_real_domain(char *domain_name)
{
	memset(domain_name, 0, HOST_DOMAIN_LEN + 1);
	strncpy(domain_name, m_remote_domain, HOST_DOMAIN_LEN);
}

void CClient::free()
{
	if (this->is_freeing())
	{
		/*is freeing, return to avoid relog*/
		return;
	}

	int status = this->get_client_status();

	if (status != SOCKS_CONNECTED && status != SOCKS_CONNECTED_FAILED)
	{
		g_syslogMgr->add_syslog(L_WARN, "client(%s/%u/fd%d) closed when in %s", m_ipstr, m_port, 
    				m_fd, g_socks_status_desc[status]);
	}

	/*call free of parent*/
	CBaseClient::free();
}

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
#include "CRemoteServerPool.h"
#include "relay_pkt_def.h"
#include "CSyslogMgr.h"

CRemoteServerPool *g_remoteSrvPool = NULL;

void CRemoteServerPool::status_check()
{
	for (int ii = 0; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);

		if (m_conns_array[ii].connObj == NULL)
		{
			proxy_cfg_t* cfginfo = proxy_cfg_get();
			CRemoteServer *rmtSrv = new CRemoteServer(cfginfo->vpn_ip, cfginfo->vpn_port);
	        if(0 != rmtSrv->init())
	        {
	        	delete rmtSrv;
	        }
	        else
	        {
				rmtSrv->set_self_pool_index(ii);
	        	this->add_conn_obj(ii, (CNetRecv*)rmtSrv);
	        }
		}
		else
		{
			CRemoteServer *rmtSrv = (CRemoteServer*)m_conns_array[ii].connObj;
			if (rmtSrv->is_connected())
			{
				if (rmtSrv->is_authed())
				{
					uint64_t cur_time = util_get_cur_time();

					if (cur_time - rmtSrv->m_latest_auth_time >= 60)
					{
						rmtSrv->send_auth_quest_msg();
						rmtSrv->m_latest_auth_time = util_get_cur_time();
					}
				}
				else
				{
					uint64_t cur_time = util_get_cur_time();

					if (cur_time - rmtSrv->m_latest_auth_time >= 6)
					{
						rmtSrv->send_auth_quest_msg();
						rmtSrv->m_latest_auth_time = util_get_cur_time();
					}
				}
			}
		}

		this->unlock_index(ii);
	}
}

void CRemoteServerPool::let_re_auth()
{
	for (int ii = 0; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);

		CRemoteServer *rmtSrv = (CRemoteServer*)m_conns_array[ii].connObj;
		if (rmtSrv != NULL && rmtSrv->is_authed())
		{
			/*server not online already, reset authed*/
			rmtSrv->reset_authed();
		}

		this->unlock_index(ii);
	}
}

int CRemoteServerPool::get_active_conn_obj()
{
	int ret = 0;
	CRemoteServer *rmtSrv = NULL;

	for (int ii = m_cur_index; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);
		rmtSrv = (CRemoteServer*)m_conns_array[ii].connObj;
		if (NULL == rmtSrv)
		{
			this->unlock_index(ii);
			continue;
		}

		if (rmtSrv->is_authed())
		{
			this->unlock_index(ii);
			ret = ii;
			break; 
		}
		this->unlock_index(ii);
	}

	if (-1 != ret)
	{
		m_cur_index++;
		if (m_cur_index == m_max_conn_cnt)
		{
			m_cur_index = 0;
		}
	}

	return ret;
}

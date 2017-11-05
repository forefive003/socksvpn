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

void CRemoteServerPool::print_statistic(FILE* pFd)
{
	for (int ii = 0; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);

		if (m_conns_array[ii].connObj != NULL)
		{
			((CRemoteServer*)m_conns_array[ii].connObj)->print_statistic(pFd);
		}

		this->unlock_index(ii);
	}
}

void CRemoteServerPool::status_check()
{
	for (int ii = 0; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);

		CRemoteServer *rmtSrv = (CRemoteServer*)this->get_conn_obj(ii);
		if (rmtSrv == NULL)
		{
			proxy_cfg_t* cfginfo = proxy_cfg_get();
			rmtSrv = new CRemoteServer(cfginfo->vpn_ip, cfginfo->vpn_port);
			int index = this->add_conn_obj((CNetRecv*)rmtSrv);
        	if (-1 == index)
        	{
        		_LOG_ERROR("fail to add new conn obj, index %d", ii);
        		delete rmtSrv;
        	}
        	else
        	{
        		/*如果先init,在设置线程索引,会导致assert异常*/        		
        		rmtSrv->set_self_pool_index(index);
		        if(0 != rmtSrv->init())
		        {
		        	delete rmtSrv;
		        	this->del_conn_obj(index);
		        }
        	}
		}
		else
		{
			if (rmtSrv->is_connected())
			{
				if (rmtSrv->is_authed())
				{
					uint64_t cur_time = util_get_cur_time();

					if (cur_time - rmtSrv->m_latest_auth_time >= 60)
					{
						rmtSrv->send_auth_quest_msg();
					}
				}
				else
				{
					uint64_t cur_time = util_get_cur_time();

					if (cur_time - rmtSrv->m_latest_auth_time >= 6)
					{
						rmtSrv->send_auth_quest_msg();
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

		CRemoteServer *rmtSrv = (CRemoteServer*)this->get_conn_obj(ii);
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
    int ret_index = -1;
    int min_conn_cnt = 0;
    int tmp_conn_cnt = 0;

	CRemoteServer *rmtSrv = NULL;

	for (int ii = 0; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);

		rmtSrv = (CRemoteServer*)this->get_conn_obj(ii);
		if (NULL == rmtSrv)
		{
			this->unlock_index(ii);
			continue;
		}

		if (!rmtSrv->is_authed())
		{
			this->unlock_index(ii);
			continue;
		}

		tmp_conn_cnt = g_remoteSrvPool->get_index_session_cnt(ii);
		if (0 == tmp_conn_cnt)
        {
            ret_index = ii;
            this->unlock_index(ii);
            break;
        }
        
        if (0 == min_conn_cnt)
        {
            min_conn_cnt = tmp_conn_cnt;
            ret_index = ii;
        }
        else if(min_conn_cnt > tmp_conn_cnt)
        {
            min_conn_cnt = tmp_conn_cnt;
            ret_index = ii;
        }
        
		this->unlock_index(ii);
	}
	
	return ret_index;
}

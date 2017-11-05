#include <netinet/tcp.h>
#include "commtype.h"
#include "logproc.h"
#include "CNetRecv.h"
#include "relay_pkt_def.h"
#include "netpool.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CClient.h"
#include "CRemote.h"
#include "CLocalServer.h"
#include "CLocalServerPool.h"
#include "socketwrap.h"
#include "socks_server.h"
#include "CSocksMem.h"

CLocalServerPool *g_localSrvPool = NULL;

void CLocalServerPool::print_statistic(FILE* pFd)
{
	for (int ii = 0; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);

		if (m_conns_array[ii].connObj != NULL)
		{
			((CLocalServer*)m_conns_array[ii].connObj)->print_statistic(pFd);
		}

		this->unlock_index(ii);
	}
}

void CLocalServerPool::status_check()
{
	uint64_t cur_time = util_get_cur_time();
	
	for (int ii = 0; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);

		if (m_conns_array[ii].connObj == NULL)
		{
			CLocalServer *localSrv = new CLocalServer(g_relay_ip, g_relay_port);
        	int index = this->add_conn_obj((CNetRecv*)localSrv);
        	if (-1 == index)
        	{
        		_LOG_ERROR("fail to add new conn obj, index %d", ii);
        		delete localSrv;
        	}
        	else
        	{
        		/*如果先init,在设置线程索引,会导致assert异常*/        		
        		localSrv->set_self_pool_index(index);
		        if(0 != localSrv->init())
		        {
		        	delete localSrv;
		        	this->del_conn_obj(index);
		        }
        	}
		}
		else
		{
			CLocalServer *localSrv = (CLocalServer*)m_conns_array[ii].connObj;
			if (localSrv->is_connected())
			{
				if (cur_time - localSrv->m_latest_alive_time >= 60)
				{
					localSrv->send_keepalive();
					localSrv->m_latest_alive_time = util_get_cur_time();
				}
			}
		}

		this->unlock_index(ii);
	}
}

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
	for (int ii = 0; ii < m_max_conn_cnt; ii++)
	{
		this->lock_index(ii);

		if (m_conns_array[ii].connObj == NULL)
		{
			CLocalServer *localSrv = new CLocalServer(g_relay_ip, g_relay_port);
	        if(0 != localSrv->init())
	        {
	        	delete localSrv;
	        }
	        else
	        {
	        	int index = this->add_conn_obj((CNetRecv*)localSrv);
	        	if (-1 == index)
	        	{
	        		_LOG_ERROR("fail to add new conn obj, index %d", ii);
	        		delete rmtSrv;
	        	}
	        	else
	        	{
	        		localSrv->set_self_pool_index(index);
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

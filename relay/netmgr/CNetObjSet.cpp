
#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "CNetRecv.h"
#include "CClient.h"
#include "CRemote.h"
#include "CClientNet.h"
#include "CSocksSrv.h"
#include "CNetObjPool.h"
#include "CNetObjSet.h"
#include "CNetObjMgr.h"

#include "socks_relay.h"

BOOL CNetObjSet::is_self(uint32_t pub_ipaddr, uint32_t private_ipaddr)
{
    if( (pub_ipaddr == m_ipaddr) && (private_ipaddr == m_private_ipaddr))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL CNetObjSet::is_self(const char* pub_ipaddr_str, const char* private_ipaddr_str)
{
    if( (0 == strncmp(m_ipstr, pub_ipaddr_str, HOST_IP_LEN)
        && (0 == strncmp(m_private_ipstr, private_ipaddr_str, HOST_IP_LEN))))
    {
        return TRUE;
    }

    return FALSE;
}

void CNetObjSet::lock()
{
#ifdef _WIN32
    while (InterlockedExchange(&m_data_lock, 1) == 1){
        sleep_s(0);
    }
#else
    pthread_spin_lock(&m_data_lock);
#endif
}

void CNetObjSet::unlock()
{
#ifdef _WIN32
    InterlockedExchange(&m_data_lock, 0);
#else
    pthread_spin_unlock(&m_data_lock);
#endif
}

int CNetObjSet::get_server_obj_cnt()
{
	return m_conn_list.size();
}

int CNetObjSet::add_server_obj(int index)
{
    this->lock();
    m_conn_list.push_back(index);
    this->unlock();
	return 0;
}

void CNetObjSet::del_server_obj(int index)
{
	SRV_INDEX_LIST_Itr itr;
    int pool_index = 0;

    this->lock();

    /*handle accept fd firstly*/
    for (itr = m_conn_list.begin();
            itr != m_conn_list.end(); )
    {
        pool_index = *itr;

        if (pool_index == index)
        {
        	itr = m_conn_list.erase(itr);
        }
        else
        {
            itr++;
        }
	}

    this->unlock();
}

void CClientNetSet::print_statistic(FILE *pFd)
{
	SRV_INDEX_LIST_Itr itr;
    int pool_index = 0;
    CClientNet *clientNet = NULL;

    fprintf(pFd, "###CLIENT (pub_ip %s, pri %s, cnt %d):\n", 
            m_ipstr, m_private_ipstr, (int)m_conn_list.size());

    this->lock();
    
    /*handle accept fd firstly*/
    for (itr = m_conn_list.begin();
            itr != m_conn_list.end(); 
            itr++)
    {
        pool_index = *itr;

        g_clientNetPool->lock_index(pool_index);
        clientNet = (CClientNet*)g_clientNetPool->get_conn_obj(pool_index);
        if (NULL != clientNet)
        {
            fprintf(pFd, "index %d, session cnt %d:\n", pool_index, 
                    g_clientNetPool->get_index_session_cnt(pool_index));
        	clientNet->print_statistic(pFd);
        }
        g_clientNetPool->unlock_index(pool_index);        
	}

    this->unlock();
}
void CClientNetSet::aged_netobj()
{
    return;
}

void CSocksNetSet::set_srv_cfg(CServerCfg *srvCfg)
{
    this->m_srvCfg = *srvCfg;
}

bool CSocksNetSet::user_authen(const char *username, const char *passwd)
{
    for (int ii = 0; ii < m_srvCfg.m_acct_cnt; ii++)
    {
        if ( 0 == strncmp(m_srvCfg.m_acct_infos[ii].username, username, MAX_USERNAME_LEN)
            && 0 == strncmp(m_srvCfg.m_acct_infos[ii].passwd, passwd, MAX_PASSWD_LEN))
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

int CSocksNetSet::get_active_socks_server()
{
    SRV_INDEX_LIST_Itr itr;
    int pool_index = 0;

    int ret_index = -1;
    int min_conn_cnt = 0;
    int tmp_conn_cnt = 0;

    this->lock();
    
    for (itr = m_conn_list.begin();
            itr != m_conn_list.end(); )
    {
        pool_index = *itr;

        /*maybe timenode deleted in callback func, after that, itr not valid*/
        itr++;

        g_socksNetPool->lock_index(pool_index);

        if (g_socksNetPool->get_conn_obj(pool_index) == NULL)
        {
            g_socksNetPool->unlock_index(pool_index);
            continue;
        }

        tmp_conn_cnt = g_socksNetPool->get_index_session_cnt(pool_index);
        if (0 == tmp_conn_cnt)
        {
            ret_index = pool_index;
            g_socksNetPool->unlock_index(pool_index);
            break;
        }

        if (0 == min_conn_cnt)
        {
            min_conn_cnt = tmp_conn_cnt;
            ret_index = pool_index;
        }
        else if(min_conn_cnt > tmp_conn_cnt)
        {
            min_conn_cnt = tmp_conn_cnt;
            ret_index = pool_index;
        }

        g_socksNetPool->unlock_index(pool_index);        
    }

    this->unlock();   

    return ret_index;
}

void CSocksNetSet::close_all_socks_server()
{
    SRV_INDEX_LIST_Itr itr;
    int pool_index = 0;
    CSocksSrv *socksSrv = NULL;

    this->lock();
    
    /*handle accept fd firstly*/
    for (itr = m_conn_list.begin();
            itr != m_conn_list.end(); )
    {
        pool_index = *itr;

        /*maybe timenode deleted in callback func, after that, itr not valid*/
        itr++;

        g_socksNetPool->lock_index(pool_index);
        socksSrv = (CSocksSrv*)g_socksNetPool->get_conn_obj(pool_index);
        if (NULL != socksSrv)
        {
            socksSrv->free();
        }
        g_socksNetPool->unlock_index(pool_index);        
    }

    this->unlock();
}

void CSocksNetSet::print_statistic(FILE *pFd)
{
    SRV_INDEX_LIST_Itr itr;
    int pool_index = 0;
    CSocksSrv *socksSrv = NULL;

    fprintf(pFd, "###SERVER (pub_ip %s, pri %s, cnt %d):\n", 
            m_ipstr, m_private_ipstr, (int)m_conn_list.size());

    this->lock();
    
    for (itr = m_conn_list.begin();
            itr != m_conn_list.end(); 
            itr++)
    {
        pool_index = *itr;

        g_socksNetPool->lock_index(pool_index);
        socksSrv = (CSocksSrv*)g_socksNetPool->get_conn_obj(pool_index);
        if (NULL != socksSrv)
        {
            fprintf(pFd, "    index %d, session cnt %d:\n", pool_index, 
                    g_socksNetPool->get_index_session_cnt(pool_index));
            socksSrv->print_statistic(pFd);
        }
        g_socksNetPool->unlock_index(pool_index);        
    }

    this->unlock();
}

void CSocksNetSet::aged_netobj()
{
    return;
}

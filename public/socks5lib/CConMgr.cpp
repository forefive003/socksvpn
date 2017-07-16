#include "commtype.h"
#include "logproc.h"
#include "CBaseObj.h"
#include "CBaseConn.h"
#include "CConMgr.h"
#include "CSocksMem.h"

CConnMgr *g_ConnMgr = NULL;

CConnMgr::CConnMgr()
{
    m_conn_cnt = 0;
    MUTEX_SETUP(m_obj_lock);
}

CConnMgr::~CConnMgr()
{
    MUTEX_CLEANUP(m_obj_lock);
}


CConnMgr* CConnMgr::instance()
{
    static CConnMgr *connMgr = NULL;

    if(connMgr == NULL)
    {
        connMgr = new CConnMgr();
    }
    return connMgr;
}

CBaseConnection* CConnMgr::get_conn_by_client(uint32_t client_ip, uint16_t client_port,
        uint32_t client_inner_ip, uint16_t client_inner_port)
{
    CONN_LIST_Itr itr;
    CBaseConnection *pConn = NULL;

    MUTEX_LOCK(m_obj_lock);
    for (itr = m_conn_objs.begin();
            itr != m_conn_objs.end();
            itr++)
    {
        pConn = *itr;

        MUTEX_LOCK(pConn->m_remote_lock);
        if (NULL == pConn->m_client)
        {
            MUTEX_UNLOCK(pConn->m_remote_lock);
            continue;
        }
        
        if (pConn->m_client->is_self(client_ip, client_port, client_inner_ip, client_inner_port))
        {
            MUTEX_UNLOCK(pConn->m_remote_lock);
            pConn->inc_ref();
            MUTEX_UNLOCK(m_obj_lock);
            return pConn;
        }
        MUTEX_UNLOCK(pConn->m_remote_lock);
    }
    MUTEX_UNLOCK(m_obj_lock);
    return NULL;
}

int CConnMgr::add_conn(CBaseConnection *pConn)
{
    MUTEX_LOCK(m_obj_lock);
    m_conn_objs.push_back(pConn);
    m_conn_cnt++;
    MUTEX_UNLOCK(m_obj_lock);

    _LOG_INFO("add connection");
    return 0;
}

void CConnMgr::del_conn(CBaseConnection *pConn)
{
    MUTEX_LOCK(m_obj_lock);
    m_conn_objs.remove(pConn);
    m_conn_cnt--;
    MUTEX_UNLOCK(m_obj_lock);

    _LOG_INFO("del connection");
}

void CConnMgr::free_all_conn()
{
    _LOG_INFO("free all connections");

    MUTEX_LOCK(m_obj_lock);

    CONN_LIST_Itr itr;

    for (itr = m_conn_objs.begin();
            itr != m_conn_objs.end();
            )
    {
        (*itr)->destroy();

        itr = m_conn_objs.erase(itr);
    }
    MUTEX_UNLOCK(m_obj_lock);
}

uint32_t CConnMgr::get_cur_conn_cnt()
{
    return m_conn_cnt;
}

void CConnMgr::print_statistic(FILE *pFd, bool is_detail)
{
    fprintf(pFd, "CONNECTION-STAT: cur conn %d, cur pool used %d\n", 
        m_conn_cnt, socks_mem_get_used_cnt());

    if (false == is_detail)
    {
        fprintf(pFd, "\n");
        return;
    }

    MUTEX_LOCK(m_obj_lock);

    CONN_LIST_Itr itr;

    for (itr = m_conn_objs.begin();
            itr != m_conn_objs.end();
            itr++)
    {
        (*itr)->print_statistic(pFd);
    }
    MUTEX_UNLOCK(m_obj_lock);

    fprintf(pFd, "\n");

    return;   
}

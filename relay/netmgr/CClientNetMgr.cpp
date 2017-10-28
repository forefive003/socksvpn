#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "CNetRecv.h"
#include "CClientNet.h"
#include "CClientNetMgr.h"
#include "socks_relay.h"


CClientNetMgr *g_ClientNetMgr = NULL;

CClientNetMgr::CClientNetMgr()
{
    MUTEX_SETUP(m_obj_lock);
}

CClientNetMgr::~CClientNetMgr()
{
    MUTEX_CLEANUP(m_obj_lock);
}


CClientNetMgr* CClientNetMgr::instance()
{
    static CClientNetMgr *clientNetMgr = NULL;

    if(clientNetMgr == NULL)
    {
        clientNetMgr = new CClientNetMgr();
    }
    return clientNetMgr;
}

int CClientNetMgr::add_client_server(CClientNet *clientNet)
{
    MUTEX_LOCK(m_obj_lock);
    m_client_objs.push_back(clientNet);
    MUTEX_UNLOCK(m_obj_lock);

    _LOG_INFO("add new client server %s:%u", clientNet->m_ipstr, clientNet->m_port);
    return 0;
}

void CClientNetMgr::del_client_server(CClientNet *clientNet)
{
    _LOG_INFO("del client server %s:%u", clientNet->m_ipstr, clientNet->m_port);

    MUTEX_LOCK(m_obj_lock);
    m_client_objs.remove(clientNet);
    MUTEX_UNLOCK(m_obj_lock);
}

CClientNet* CClientNetMgr::get_client_server(uint32_t pub_ipaddr, uint16_t pub_port)
{
    CLINTNET_LIST_Itr itr;
    CClientNet *clientNet = NULL;

    for (itr = m_client_objs.begin();
            itr != m_client_objs.end();
            itr++)
    {
        clientNet = *itr;
        if (clientNet->is_self(pub_ipaddr, pub_port))
        {
            return clientNet;
        }
    }
    return NULL;
}

void CClientNetMgr::lock()
{
    MUTEX_LOCK(m_obj_lock);
}

void CClientNetMgr::unlock()
{
    MUTEX_UNLOCK(m_obj_lock);
}


void CClientNetMgr::print_statistic(FILE *pFd)
{
    CLINTNET_LIST_Itr itr;
    CClientNet *clientNet = NULL;
    
    fprintf(pFd, "CLIENT-STAT:\n");

    MUTEX_LOCK(m_obj_lock);

    for (itr = m_client_objs.begin();
            itr != m_client_objs.end();
            itr++)
    {
        clientNet = *itr;
        
        clientNet->print_statistic(pFd);
    }
    MUTEX_UNLOCK(m_obj_lock);

    fprintf(pFd, "\n");

    return;   
}


void CClientNetMgr::aged_client_server()
{
    CLINTNET_LIST_Itr itr;
    CClientNet *clientNet = NULL;
    
    uint64_t cur_time = util_get_cur_time();

    MUTEX_LOCK(m_obj_lock);
    for (itr = m_client_objs.begin();
            itr != m_client_objs.end();
            )
    {
        clientNet = *itr;
        itr++;
        
        if (clientNet->m_update_time < cur_time
            && (cur_time - clientNet->m_update_time >= 30))
        {
            _LOG_ERROR("client server %s:%u, fd %d aged", 
                clientNet->m_ipstr, clientNet->m_port, clientNet->m_fd);
            
            clientNet->free();
        }
    }
    MUTEX_UNLOCK(m_obj_lock);    
    return;
}

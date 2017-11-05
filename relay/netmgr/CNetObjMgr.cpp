#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "CNetRecv.h"
#include "CClientNet.h"
#include "CNetObjPool.h"
#include "CNetObjSet.h"
#include "CNetObjMgr.h"

#include "socks_relay.h"


CClientNetMgr *g_ClientNetMgr = NULL;
CSocksNetMgr *g_SocksSrvMgr = NULL;

CNetObjMgr::CNetObjMgr()
{
#ifndef _WIN32
    pthread_mutexattr_t mux_attr;
    memset(&mux_attr, 0, sizeof(mux_attr));
    pthread_mutexattr_settype(&mux_attr, PTHREAD_MUTEX_RECURSIVE);
    MUTEX_SETUP_ATTR(m_obj_lock, &mux_attr);
#else
    MUTEX_SETUP(m_obj_lock);
#endif
}

CNetObjMgr::~CNetObjMgr()
{
    MUTEX_CLEANUP(m_obj_lock);
}

void CNetObjMgr::lock()
{
    MUTEX_LOCK(m_obj_lock);
}

void CNetObjMgr::unlock()
{
    MUTEX_UNLOCK(m_obj_lock);
}

int CNetObjMgr::add_netobj_set(CNetObjSet *serverSet)
{
    MUTEX_LOCK(m_obj_lock);
    m_netset_objs.push_back(serverSet);
    MUTEX_UNLOCK(m_obj_lock);

    _LOG_INFO("add new server set at %s", serverSet->m_ipstr);
    return 0;
}

void CNetObjMgr::del_netobj_set(CNetObjSet *serverSet)
{
    _LOG_INFO("del server set at %s", serverSet->m_ipstr);

    MUTEX_LOCK(m_obj_lock);
    m_netset_objs.remove(serverSet);
    MUTEX_UNLOCK(m_obj_lock);
}

CNetObjSet* CNetObjMgr::get_netobj_set(uint32_t pub_ipaddr, uint32_t private_ipaddr)
{
    NETOBJ_SET_LIST_Itr itr;
    CNetObjSet *serverSet = NULL;

    for (itr = m_netset_objs.begin();
            itr != m_netset_objs.end();
            itr++)
    {
        serverSet = *itr;
        if (serverSet->is_self(pub_ipaddr, private_ipaddr))
        {
            return serverSet;
        }
    }
    return NULL;
}

CNetObjSet* CNetObjMgr::get_netobj_set(char *pub_ipaddr_str, char *private_ipaddr_str)
{
    NETOBJ_SET_LIST_Itr itr;
    CNetObjSet *serverSet = NULL;

    for (itr = m_netset_objs.begin();
            itr != m_netset_objs.end();
            itr++)
    {
        serverSet = *itr;
        if (serverSet->is_self(pub_ipaddr_str, private_ipaddr_str))
        {
            return serverSet;
        }
    }
    return NULL;
}

void CNetObjMgr::aged_netobj()
{
    NETOBJ_SET_LIST_Itr itr;
    CNetObjSet *netObjSet = NULL;
    
    MUTEX_LOCK(m_obj_lock);
    for (itr = m_netset_objs.begin();
            itr != m_netset_objs.end();
            )
    {
        netObjSet = *itr;
        itr++;
        
        netObjSet->aged_netobj();
    }
    MUTEX_UNLOCK(m_obj_lock);    
    return;
}

int CNetObjMgr::add_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr)
{
    return 0;
}

void CNetObjMgr::del_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr)
{
    CNetObjSet *netobjSet = NULL;

    this->lock();

    netobjSet = this->get_netobj_set(pub_ipaddr, private_ipaddr);
    if (NULL == netobjSet)
    {
        _LOG_WARN("no netobjSet when del netobj (0x%x/0x%x), index %d",
            pub_ipaddr, private_ipaddr, index);
    }
    else
    {
        netobjSet->del_server_obj(index);
        if (netobjSet->get_server_obj_cnt() == 0)
        {
            /*没有元素则删除*/
            this->del_netobj_set(netobjSet);
            delete netobjSet;
        }
    }
    
    this->unlock();
}

void CNetObjMgr::print_statistic(FILE *pFd)
{
    NETOBJ_SET_LIST_Itr itr;
    CNetObjSet *netObjSet = NULL;
    
    MUTEX_LOCK(m_obj_lock);
    for (itr = m_netset_objs.begin();
            itr != m_netset_objs.end();
            itr++)
    {
        netObjSet = *itr;        
        netObjSet->print_statistic(pFd);
    }
    MUTEX_UNLOCK(m_obj_lock);

    fprintf(pFd, "\n");

    return;   
}

int CClientNetMgr::add_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr)
{
    this->lock();

    CClientNetSet *clientNetSet = NULL;
    clientNetSet = (CClientNetSet*)this->get_netobj_set(pub_ipaddr, private_ipaddr);
    if (NULL == clientNetSet)
    {
        clientNetSet = new CClientNetSet(pub_ipaddr, private_ipaddr);
        assert(clientNetSet != NULL);
        /*添加到管理中*/
        this->add_netobj_set((CNetObjSet*)clientNetSet);
    }

    clientNetSet->add_server_obj(index);
    this->unlock(); 

    return 0; 
}

int CSocksNetMgr::add_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr, CServerCfg *srvCfg)
{
    this->lock();

    CSocksNetSet *socksNetSet = NULL;
    socksNetSet = (CSocksNetSet*)this->get_netobj_set(pub_ipaddr, private_ipaddr);
    if (NULL == socksNetSet)
    {
        socksNetSet = new CSocksNetSet(pub_ipaddr, private_ipaddr, srvCfg);
        assert(socksNetSet != NULL);

        /*添加到管理中*/
        this->add_netobj_set((CNetObjSet*)socksNetSet);
    }

    socksNetSet->add_server_obj(index);
    this->unlock(); 

    return 0; 
}

int CSocksNetMgr::get_running_socks_servers(int *serv_array)
{
    int cnt = 0;

    NETOBJ_SET_LIST_Itr itr;
    CSocksNetSet *socksNetSet = NULL;
        
    this->lock(); 

    for (itr = m_netset_objs.begin();
            itr != m_netset_objs.end();
            itr++)
    {
        socksNetSet = (CSocksNetSet*)*itr;
        
        serv_array[cnt] = socksNetSet->m_ipaddr;
        cnt++;
    }

    this->unlock(); 
    return cnt;  
}

CNetObjSet* CSocksNetMgr::get_socks_server_by_auth(uint32_t srv_pub_ipaddr, 
        const char *username, const char *passwd)
{
    NETOBJ_SET_LIST_Itr itr;
    CSocksNetSet *socksNetSet = NULL;
    
    for (itr = m_netset_objs.begin();
            itr != m_netset_objs.end();
            itr++)
    {
        socksNetSet = (CSocksNetSet*)*itr;
        
        if (socksNetSet->m_ipaddr != srv_pub_ipaddr)
        {
            continue;
        }

        if (is_relay_need_platform())
        {
            if(socksNetSet->user_authen(username, passwd))
            {
                return (CNetObjSet*)socksNetSet;
            }
        }
        else
        {
            /*没有平台,不需要认证*/
            return (CNetObjSet*)socksNetSet;
        }
    }

    return NULL;  
}

int CSocksNetMgr::get_active_socks_server(uint32_t pub_ipaddr, uint32_t private_ipaddr)
{
    NETOBJ_SET_LIST_Itr itr;
    CSocksNetSet *serverSet = NULL;

    for (itr = m_netset_objs.begin();
            itr != m_netset_objs.end();
            itr++)
    {
        serverSet = (CSocksNetSet*)*itr;
        if (serverSet->is_self(pub_ipaddr, private_ipaddr))
        {
            return serverSet->get_active_socks_server();
        }
    }
    
    return -1;
}

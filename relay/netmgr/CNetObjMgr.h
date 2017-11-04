#ifndef _RELAY_CLIETNET_MGR_H
#define _RELAY_CLIETNET_MGR_H

#include <list>
#include "CNetObjSet.h"

class CNetObjMgr
{
    friend class CClientNetMgr;
    friend class CSocksNetMgr;

public:
    CNetObjMgr();
    virtual ~CNetObjMgr();

public:
    int add_netobj_set(CNetObjSet *serverSet);
    void del_netobj_set(CNetObjSet *serverSet);
    CNetObjSet* get_netobj_set(uint32_t pub_ipaddr, uint32_t private_ipaddr);
    CNetObjSet* get_netobj_set(char *pub_ipaddr_str, char *private_ipaddr_str);

    virtual void aged_netobj();
    virtual int add_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr);
    virtual void del_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr);

    void print_statistic(FILE *pFd);

    void lock();
    void unlock();    

private:
    MUTEX_TYPE m_obj_lock;
    NETOBJ_SET_LIST m_netset_objs;
};

class CClientNetMgr : public CNetObjMgr
{
public:
    int add_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr);
};

class CSocksNetMgr : public CNetObjMgr
{
public:
    int add_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr)
    {
        return 0;
    }
    int add_netobj(int index, uint32_t pub_ipaddr, uint32_t private_ipaddr, CServerCfg *srvCfg);

    int get_running_socks_servers(uint32_t pub_ipaddr, uint32_t private_ipaddr,
                        int *serv_array);
    CNetObjSet* get_socks_server_by_auth(uint32_t srv_pub_ipaddr, 
                const char *username, const char *passwd);
    int get_active_socks_server(uint32_t pub_ipaddr, uint32_t private_ipaddr);
};

extern CClientNetMgr *g_ClientNetMgr;
extern CSocksNetMgr *g_SocksSrvMgr;

#endif

#ifndef _RELAY_CLIETNET_MGR_H
#define _RELAY_CLIETNET_MGR_H

#include <list>
#include "CClientNet.h"

typedef std::list<CClientNet*> CLINTNET_LIST;
typedef CLINTNET_LIST::iterator CLINTNET_LIST_Itr;

class CClientNetMgr
{
public:
    CClientNetMgr();
    virtual ~CClientNetMgr();
    static CClientNetMgr* instance();

public:
    int add_client_server(CClientNet *rsocks);
    void del_client_server(CClientNet *rsocks);
    CClientNet* get_client_server(uint32_t pub_ipaddr, uint16_t pub_port);

    void lock();
    void unlock();
    void print_statistic(FILE *pFd);

private:
    MUTEX_TYPE m_obj_lock;
    CLINTNET_LIST m_client_objs;
};

extern CClientNetMgr *g_ClientNetMgr;

#endif

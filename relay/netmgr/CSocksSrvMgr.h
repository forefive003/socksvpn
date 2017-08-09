#ifndef _RELAY_SOCKSSERVER_MGR_H
#define _RELAY_SOCKSSERVER_MGR_H

#include <list>
#include "CSocksSrv.h"

typedef std::list<CSocksSrv*> RSOCKS_LIST;
typedef RSOCKS_LIST::iterator RSOCKS_LIST_Itr;

class CSocksSrvMgr
{
public:
    CSocksSrvMgr();
    virtual ~CSocksSrvMgr();
    static CSocksSrvMgr* instance();

public:
    int add_socks_server(CSocksSrv *rsocks);
    void del_socks_server(CSocksSrv *rsocks);
    CSocksSrv* get_socks_server_by_innnerip(uint32_t pub_ipaddr, uint32_t inner_ipaddr);
    CSocksSrv* get_socks_server_by_innnerip(const char* pub_ipstr, const char* inner_ipstr);
    CSocksSrv* get_socks_server_by_user(uint32_t pub_ipaddr, char *username);

    void lock();
    void unlock();
    void print_statistic(FILE *pFd);
    #if 0
    int output_socks_server(char *resp_buf, int buf_len);
    #endif
    
private:
    MUTEX_TYPE m_obj_lock;
    RSOCKS_LIST m_rsocks_objs;
};

extern CSocksSrvMgr *g_SocksSrvMgr;

#endif


#ifndef CSERVER_CFG_H_
#define CSERVER_CFG_H_

#include <list>
#include "ipparser.h"
#include "CWebApi.h"

typedef std::list<CServerCfg*> CSERVCFG_LIST;
typedef CSERVCFG_LIST::iterator CSERVCFG_LIST_Itr;

class CServCfgMgr
{
public:
    CServCfgMgr();
    virtual ~CServCfgMgr();
    static CServCfgMgr* instance();

    int add_server_cfg(CServerCfg *srvCfg);
    void del_server_cfg(CServerCfg *srvCfg);

    int server_online_handle(char *sn, char *pub_ip, char *pri_ip, CServerCfg *srvCfg);
    void server_offline_handle(char *sn, char *pub_ip, char *pri_ip);
    
    void server_post_keepalive();
    
private:
    CServerCfg* find_server_cfg(char *sn);
    CServerCfg* add_server_cfg_by_pkt(char *sn, char *pub_ip, char *pri_ip);

private:
    MUTEX_TYPE m_obj_lock;
    CSERVCFG_LIST m_objs;
};

extern CServCfgMgr *g_SrvCfgMgr;

#endif

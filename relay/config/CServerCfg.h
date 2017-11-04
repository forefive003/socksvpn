
#ifndef CSERVER_CFG_H_
#define CSERVER_CFG_H_

#include <list>
#include "engine_ip.h"
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
    void del_server_cfg(char *sn);
    int get_server_cfg(char *sn, CServerCfg* srvCfg);

    int set_server_online(char *sn, char *pub_ip, char *pri_ip);
    void set_server_offline(char *sn, char *pub_ip, char *pri_ip);
    
    void server_post_keepalive();
    
private:
    CServerCfg* find_server_cfg(char *sn);
    void add_server_cfg_by_pkt(char *sn, char *pub_ip, char *pri_ip);

private:
    MUTEX_TYPE m_obj_lock;
    CSERVCFG_LIST m_objs;
};

extern CServCfgMgr *g_SrvCfgMgr;

#endif

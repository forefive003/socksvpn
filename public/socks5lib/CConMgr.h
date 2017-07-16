
#ifndef _NET_CONNECTION_MGR_H
#define _NET_CONNECTION_MGR_H

#include <list>

typedef std::list<CBaseConnection*> CONN_LIST;
typedef CONN_LIST::iterator CONN_LIST_Itr;

class CConnMgr
{
public:
    CConnMgr();
    virtual ~CConnMgr();
    static CConnMgr* instance();
    
public:
    int add_conn(CBaseConnection *pConn);
    void del_conn(CBaseConnection *pConn);
    void free_all_conn();

    uint32_t get_cur_conn_cnt();

    void print_statistic(FILE *pFd, bool is_detail);
    CBaseConnection* get_conn_by_client(uint32_t client_ip, uint16_t client_port,
    	uint32_t client_inner_ip, uint16_t client_inner_port);
private:
    MUTEX_TYPE m_obj_lock;
    CONN_LIST m_conn_objs;
    uint32_t m_conn_cnt;
};

extern CConnMgr *g_ConnMgr;

#endif

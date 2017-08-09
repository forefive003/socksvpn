#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "CSocksSrv.h"
#include "CSocksSrvMgr.h"
#include "socks_relay.h"


CSocksSrvMgr *g_SocksSrvMgr = NULL;

CSocksSrvMgr::CSocksSrvMgr()
{
	MUTEX_SETUP(m_obj_lock);
}

CSocksSrvMgr::~CSocksSrvMgr()
{
	MUTEX_CLEANUP(m_obj_lock);
}


CSocksSrvMgr* CSocksSrvMgr::instance()
{
    static CSocksSrvMgr *socksSrvMgr = NULL;

    if(socksSrvMgr == NULL)
    {
        socksSrvMgr = new CSocksSrvMgr();
    }
    return socksSrvMgr;
}

int CSocksSrvMgr::add_socks_server(CSocksSrv *rsocks)
{
	MUTEX_LOCK(m_obj_lock);
    m_rsocks_objs.push_back(rsocks);
    MUTEX_UNLOCK(m_obj_lock);

    _LOG_INFO("add new socks server %s:%u", rsocks->m_ipstr, rsocks->m_port);
    return 0;
}

void CSocksSrvMgr::del_socks_server(CSocksSrv *rsocks)
{
	_LOG_INFO("del socks server %s:%u", rsocks->m_ipstr, rsocks->m_port);

	MUTEX_LOCK(m_obj_lock);
    m_rsocks_objs.remove(rsocks);
    MUTEX_UNLOCK(m_obj_lock);
}

CSocksSrv* CSocksSrvMgr::get_socks_server_by_innnerip(uint32_t pub_ipaddr, uint32_t inner_ipaddr)
{
	RSOCKS_LIST_Itr itr;
    CSocksSrv *pSocksSrv = NULL;

    for (itr = m_rsocks_objs.begin();
            itr != m_rsocks_objs.end();
            itr++)
    {
        pSocksSrv = *itr;
        if (pSocksSrv->is_self(pub_ipaddr, inner_ipaddr))
        {
            return pSocksSrv;
        }
    }
    return NULL;
}


CSocksSrv* CSocksSrvMgr::get_socks_server_by_innnerip(const char* pub_ipstr, const char* inner_ipstr)
{
    RSOCKS_LIST_Itr itr;
    CSocksSrv *pSocksSrv = NULL;

    for (itr = m_rsocks_objs.begin();
            itr != m_rsocks_objs.end();
            itr++)
    {
        pSocksSrv = *itr;
        if (pSocksSrv->is_self(pub_ipstr, inner_ipstr))
        {
            return pSocksSrv;
        }
    }
    return NULL;
}

CSocksSrv* CSocksSrvMgr::get_socks_server_by_user(uint32_t pub_ipaddr, char *username)
{
	RSOCKS_LIST_Itr itr;
    CSocksSrv *pSocksSrv = NULL;

    for (itr = m_rsocks_objs.begin();
            itr != m_rsocks_objs.end();
            itr++)
    {
        pSocksSrv = *itr;
        if (pSocksSrv->is_self(pub_ipaddr, username))
        {
            return pSocksSrv;
        }
    }
    return NULL;
}

void CSocksSrvMgr::lock()
{
	MUTEX_LOCK(m_obj_lock);
}

void CSocksSrvMgr::unlock()
{
	MUTEX_UNLOCK(m_obj_lock);
}

void CSocksSrvMgr::print_statistic(FILE *pFd)
{
    RSOCKS_LIST_Itr itr;
    CSocksSrv *pSocksSrv = NULL;
    
    fprintf(pFd, "SERVER-STAT:\n");

    MUTEX_LOCK(m_obj_lock);

    for (itr = m_rsocks_objs.begin();
            itr != m_rsocks_objs.end();
            itr++)
    {
        pSocksSrv = *itr;
        
        pSocksSrv->print_statistic(pFd);
    }
    MUTEX_UNLOCK(m_obj_lock);

    fprintf(pFd, "\n");

    return;   
}

#if 0
/*
[
{"pub-ip":"202.1.1.1", "pri-ip":"192.168.1.1", "username":"hahaha", "passwd":"xxxx", "enabled":1},
{"pub-ip":"202.1.1.1", "pri-ip":"192.168.1.1", "username":"hahaha", "passwd":"xxxx", "enabled":1},
{"pub-ip":"202.1.1.1", "pri-ip":"192.168.1.1", "username":"hahaha", "passwd":"xxxx", "enabled":1}
]
*/
int CSocksSrvMgr::output_socks_server(char *resp_buf, int buf_len)
{
    RSOCKS_LIST_Itr itr;
    CSocksSrv *pSocksSrv = NULL;
    int ret_len = 0;
    int cur_pos = 0;

    char srv_info_buf[512] = {0};

    resp_buf[0] = '[';
    cur_pos++;

    MUTEX_LOCK(m_obj_lock);

    for (itr = m_rsocks_objs.begin();
            itr != m_rsocks_objs.end();
            itr++)
    {
        pSocksSrv = *itr;
        
        ret_len = pSocksSrv->output_self(srv_info_buf, 512);
        if (-1 == ret_len)
        {
            _LOG_ERROR("fail to get sock srv info, maybe buf not enough");
            break;
        }
        if (buf_len - cur_pos < ret_len)
        {
            _LOG_ERROR("fail to fill all sock srv info, maybe buf not enough");
            break;
        }

        memcpy(&resp_buf[cur_pos], srv_info_buf, ret_len);
        cur_pos += ret_len;
        resp_buf[cur_pos] = ',';
        cur_pos++;
    }
    MUTEX_UNLOCK(m_obj_lock);

    if (resp_buf[cur_pos-1] == ',')
    {
        resp_buf[cur_pos-1] = ']';
    }
    else
    {
        resp_buf[cur_pos] = ']';
        cur_pos++;
    }
    return cur_pos;
}
#endif

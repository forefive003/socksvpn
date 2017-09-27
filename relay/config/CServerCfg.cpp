#include <pthread.h>

#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "CServerCfg.h"
#include "CSocksSrv.h"
#include "CSocksSrvMgr.h"
#include "socks_relay.h"
#include "CWebApi.h"


CServCfgMgr *g_SrvCfgMgr = NULL;

CServCfgMgr::CServCfgMgr()
{
    MUTEX_SETUP(m_obj_lock);
}

CServCfgMgr::~CServCfgMgr()
{
    MUTEX_CLEANUP(m_obj_lock);
}

CServCfgMgr* CServCfgMgr::instance()
{
    static CServCfgMgr *srvCfgMgr = NULL;

    if(srvCfgMgr == NULL)
    {
        srvCfgMgr = new CServCfgMgr();
    }
    return srvCfgMgr;
}

void CServCfgMgr::server_post_keepalive()
{
    if (is_relay_need_platform() == false)
    {
        return;
    }

    CSERVCFG_LIST_Itr itr;
    CServerCfg *tmp_srvCfg = NULL;

    MUTEX_LOCK(m_obj_lock);
    for (itr = m_objs.begin();
            itr != m_objs.end();
            itr++)
    {
        tmp_srvCfg = *itr;

        if (tmp_srvCfg->m_is_online == false)
        {
            continue;
        }

        /*通知平台*/
        if(0 != g_webApi->postServerOnline(g_relaysn, tmp_srvCfg->m_sn, tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip, TRUE))
        {
            _LOG_WARN("socksserver %s, localip %s failed to post keepalive to platform", tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip);
        }
    }
    MUTEX_UNLOCK(m_obj_lock);
    return;
}

void CServCfgMgr::server_offline_handle(char *sn, char *pub_ip, char *pri_ip)
{
    if (is_relay_need_platform() == false)
    {
        return;
    }

    bool is_found = false;
    CSERVCFG_LIST_Itr itr;
    CServerCfg *tmp_srvCfg = NULL;

    MUTEX_LOCK(m_obj_lock);
    for (itr = m_objs.begin();
            itr != m_objs.end();
            itr++)
    {
        tmp_srvCfg = *itr;
        if (tmp_srvCfg->is_self(sn))
        {
            is_found = true;

            if (tmp_srvCfg->m_is_add_by_packet)
            {
                _LOG_INFO("delete socksserver %s, localip %s for offline", tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip);
  
                /*如果是基于报文创建的,直接删除*/
                m_objs.erase(itr);
                delete tmp_srvCfg;
            }
            else
            {
                /*设置离线*/
                tmp_srvCfg->set_online(FALSE);
            }

#if 0
            /*通知平台*/
            if(0 != g_webApi->postServerOnline(g_relaysn, srvCfg->m_sn, srvCfg->m_pub_ip, srvCfg->m_pri_ip, FALSE))
            {
                _LOG_WARN("socksserver %s, localip %s failed to post platform", srvCfg->m_pub_ip, srvCfg->m_pri_ip);
            }
#endif
            break;
        }
    }

    if (is_found == false)
    {
        _LOG_WARN("server(sn %s) has no server config when offline",sn);
    }
    MUTEX_UNLOCK(m_obj_lock);

    return;
}

int CServCfgMgr::server_online_handle(char *sn, char *pub_ip, char *pri_ip, CServerCfg *srvCfg)
{
    if (is_relay_need_platform() == false)
    {
        return 0;
    }

    bool is_found = false;
    CSERVCFG_LIST_Itr itr;
    CServerCfg *tmp_srvCfg = NULL;

    MUTEX_LOCK(m_obj_lock);
    for (itr = m_objs.begin();
            itr != m_objs.end();
            itr++)
    {
        tmp_srvCfg = *itr;
        if (tmp_srvCfg->is_self(sn))
        {
            is_found = true;

            /*设置在线*/
            tmp_srvCfg->set_online(TRUE);

            /*拷贝配置*/
            *srvCfg = *tmp_srvCfg;
            break;
        }
    }

    if (is_found == false)
    {
        _LOG_WARN("server(sn %s) has no server config when online",sn);
        /*添加一个配置*/
        tmp_srvCfg = add_server_cfg_by_pkt(sn, pub_ip, pri_ip);
        /*拷贝配置*/
        *srvCfg = *tmp_srvCfg;
    }
    MUTEX_UNLOCK(m_obj_lock);

    /*通知平台*/
    if(0 != g_webApi->postServerOnline(g_relaysn, srvCfg->m_sn, srvCfg->m_pub_ip, srvCfg->m_pri_ip, TRUE))
    {
        _LOG_WARN("socksserver %s, localip %s failed to post platform", srvCfg->m_pub_ip, srvCfg->m_pri_ip);
    }
    return 0;
}

CServerCfg* CServCfgMgr::find_server_cfg(char *sn)
{
	CSERVCFG_LIST_Itr itr;
    CServerCfg *srvCfg = NULL;

    for (itr = m_objs.begin();
            itr != m_objs.end();
            itr++)
    {
        srvCfg = *itr;
        if (srvCfg->is_self(sn))
        {
            return srvCfg;
        }
    }
    return NULL;
}

/*在锁内调用*/
CServerCfg* CServCfgMgr::add_server_cfg_by_pkt(char *sn, char *pub_ip, char *pri_ip)
{
    _LOG_INFO("add new server cfg by pkt: SN %s, pubip %s, priip %s", 
        sn, pub_ip, pri_ip);

    CServerCfg *tmp_srvCfg = new CServerCfg;
    m_objs.push_back(tmp_srvCfg);

    tmp_srvCfg->set_online(true);
    tmp_srvCfg->set_created_pkt_flag(true);
    tmp_srvCfg->set_sn_and_ip(sn, pub_ip, pri_ip);

    return tmp_srvCfg;
}

int CServCfgMgr::add_server_cfg(CServerCfg *srvCfg)
{
	CServerCfg *tmp_srvCfg = this->find_server_cfg(srvCfg->m_sn);
	if (NULL != tmp_srvCfg)
	{
        _LOG_INFO("modify server cfg: SN %s, pubip %s, priip %s", 
            srvCfg->m_sn, srvCfg->m_pub_ip, srvCfg->m_pri_ip);
	}
    else
    { 
        _LOG_INFO("add new server cfg: SN %s, pubip %s, priip %s", 
            srvCfg->m_sn, srvCfg->m_pub_ip, srvCfg->m_pri_ip);

        tmp_srvCfg = new CServerCfg;

        MUTEX_LOCK(m_obj_lock);
        m_objs.push_back(tmp_srvCfg);
        MUTEX_UNLOCK(m_obj_lock);
    }
    
    tmp_srvCfg->set_created_pkt_flag(false);
    tmp_srvCfg->set_sn_and_ip(srvCfg->m_sn, srvCfg->m_pub_ip, srvCfg->m_pri_ip);

    tmp_srvCfg->m_acct_cnt = srvCfg->m_acct_cnt;
    memcpy(tmp_srvCfg->m_acct_infos, srvCfg->m_acct_infos, sizeof(srvCfg->m_acct_infos));

    /*找到SocksSrv, 下发配置*/
    CSocksSrv *socksSrv = NULL;
    g_SocksSrvMgr->lock();
    socksSrv = g_SocksSrvMgr->get_socks_server_by_innnerip(tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip);
    if (NULL == socksSrv)
    {
        _LOG_WARN("socksserver %s(%s) not exist", tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip);
    }
    else
    {
        socksSrv->set_config(tmp_srvCfg);
        /*设置在线*/
        tmp_srvCfg->set_online(TRUE);
    }
    g_SocksSrvMgr->unlock();
    return 0;
}

void CServCfgMgr::del_server_cfg(char *sn)
{
    CServerCfg srvCfg;

    /*清除配置*/
    MUTEX_LOCK(m_obj_lock);
    CServerCfg *tmp_srvCfg = this->find_server_cfg(sn);
    if (NULL == tmp_srvCfg)
    {
        MUTEX_UNLOCK(m_obj_lock);
        _LOG_ERROR("sock srv(sn:%s) not exist when del", sn);
        return;
    }
    
    _LOG_INFO("del server cfg: SN %s, pubip %s, priip %s", 
        sn, tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip);

    srvCfg = *tmp_srvCfg;

    m_objs.remove(tmp_srvCfg);
    MUTEX_UNLOCK(m_obj_lock);
    delete tmp_srvCfg;

    /*通知server 关闭已有服务*/
    CSocksSrv *socksSrv = NULL;
    g_SocksSrvMgr->lock();
    socksSrv = g_SocksSrvMgr->get_socks_server_by_innnerip(srvCfg.m_pub_ip, srvCfg.m_pri_ip);
    if (NULL == socksSrv)
    {
        _LOG_WARN("socksserver %s(%s) not exist", srvCfg.m_pub_ip, srvCfg.m_pri_ip);
    }
    else
    {
        /*关闭连接*/
        socksSrv->free();
    }
    g_SocksSrvMgr->unlock();

    return;
}

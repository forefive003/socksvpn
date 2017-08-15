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
        if(0 != g_webApi->postServerOnline(tmp_srvCfg->m_sn, tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip, TRUE))
        {
            _LOG_WARN("socksserver %x, localip %s failed to post keepalive to platform", tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip);
        }
    }
    MUTEX_UNLOCK(m_obj_lock);
    return;
}

int CServCfgMgr::server_online_handle(char *sn, CServerCfg *srvCfg)
{
    if (is_relay_need_platform() == false)
    {
        return 0;
    }

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
            /*设置在线*/
            tmp_srvCfg->set_online(TRUE);

            /*拷贝配置*/
            *srvCfg = *tmp_srvCfg;
            MUTEX_UNLOCK(m_obj_lock);

            /*通知平台*/
            if(0 != g_webApi->postServerOnline(sn, srvCfg->m_pub_ip, srvCfg->m_pri_ip, TRUE))
            {
                _LOG_WARN("socksserver %x, localip %s failed to post platform", srvCfg->m_pub_ip, srvCfg->m_pri_ip);
            }
            return 0;
        }
    }
    MUTEX_UNLOCK(m_obj_lock);

    _LOG_WARN("failed to get srv config for server sn %s",sn);
    return -1;
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
    
    memcpy(tmp_srvCfg->m_sn, srvCfg->m_sn, MAX_SN_LEN);
    memcpy(tmp_srvCfg->m_pub_ip, srvCfg->m_pub_ip, IP_DESC_LEN);
    memcpy(tmp_srvCfg->m_pri_ip, srvCfg->m_pri_ip, IP_DESC_LEN);

    tmp_srvCfg->m_acct_cnt = srvCfg->m_acct_cnt;
    memcpy(tmp_srvCfg->m_acct_infos, srvCfg->m_acct_infos, sizeof(srvCfg->m_acct_infos));

    /*找到SocksSrv, 下发配置*/
    CSocksSrv *socksSrv = NULL;
    g_SocksSrvMgr->lock();
    socksSrv = g_SocksSrvMgr->get_socks_server_by_innnerip(tmp_srvCfg->m_pub_ip, tmp_srvCfg->m_pri_ip);
    if (NULL == socksSrv)
    {
        g_SocksSrvMgr->unlock();
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

void CServCfgMgr::del_server_cfg(CServerCfg *srvCfg)
{
    CServerCfg *tmp_srvCfg = this->find_server_cfg(srvCfg->m_sn);
    if (NULL != tmp_srvCfg)
    {
        _LOG_ERROR("sock srv(sn:%s) not exist when del", srvCfg->m_sn);
        return;
    }

    MUTEX_LOCK(m_obj_lock);
    m_objs.remove(tmp_srvCfg);
    MUTEX_UNLOCK(m_obj_lock);

    delete tmp_srvCfg;

    _LOG_INFO("del server cfg: SN %s, pubip %s, priip %s", 
        srvCfg->m_sn, srvCfg->m_pub_ip, srvCfg->m_pri_ip);
}

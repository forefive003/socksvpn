#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "CServCfg.h"


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

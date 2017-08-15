
#ifndef CSERVER_CFG_H_
#define CSERVER_CFG_H_

#include <list>
#include "ipparser.h"

typedef struct  server_info
{
	char username[MAX_USERNAME_LEN + 1];
	char passwd[MAX_PASSWD_LEN + 1];
	bool enabled;
}acct_info_t;

#define MAX_ACCT_PER_SERVER  32

class CServerCfg
{
public:
	CServerCfg()
	{
		memset(m_sn, 0, MAX_SN_LEN);
		memset(m_pub_ip, 0, IP_DESC_LEN);
		memset(m_pri_ip, 0, IP_DESC_LEN);

		m_acct_cnt = 0;
		memset(m_acct_infos, 0, sizeof(m_acct_infos));
	}

	CServerCfg(CServerCfg &srvCfg)
	{
		strncpy(m_sn, srvCfg.m_sn, MAX_SN_LEN);
		strncpy(m_pub_ip, srvCfg.m_pub_ip, IP_DESC_LEN);
		strncpy(m_pri_ip, srvCfg.m_pri_ip, IP_DESC_LEN);

		m_acct_cnt = srvCfg.m_acct_cnt;
		memcpy(m_acct_infos, srvCfg.m_acct_infos, sizeof(m_acct_infos));
	}

	bool is_self(char *sn)
	{
		if (strncmp(sn, m_sn, MAX_SN_LEN) == 0)
		{
			return true;
		}

		return false;
	}

public:
	acct_info_t m_acct_infos[MAX_ACCT_PER_SERVER];
	int m_acct_cnt;

	char m_sn[MAX_SN_LEN];
	char m_pub_ip[IP_DESC_LEN];
	char m_pri_ip[IP_DESC_LEN];
};


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
    int get_server_cfg(char *sn, CServerCfg *srvCfg);

private:
    CServerCfg* find_server_cfg(char *sn);
    

private:
    MUTEX_TYPE m_obj_lock;
    CSERVCFG_LIST m_objs;
};

extern CServCfgMgr *g_SrvCfgMgr;

#endif

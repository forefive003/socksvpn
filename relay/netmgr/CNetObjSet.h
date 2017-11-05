#ifndef _NETOBJ_SET_H
#define _NETOBJ_SET_H

#include "CWebApi.h"
#include "socks_relay.h"
#include "CServerCfg.h"


typedef std::list<int> SRV_INDEX_LIST;
typedef SRV_INDEX_LIST::iterator SRV_INDEX_LIST_Itr;

class CNetObjSet;
typedef std::list<CNetObjSet*> NETOBJ_SET_LIST;
typedef NETOBJ_SET_LIST::iterator NETOBJ_SET_LIST_Itr;

class CNetObjSet
{
	friend class CClientNetSet;
	friend class CSocksNetSet;
	
public:
	CNetObjSet(uint32_t ipaddr, uint32_t private_ipaddr)
	{
		m_ipaddr = ipaddr;
		m_private_ipaddr = private_ipaddr;

		if (NULL == engine_ipv4_to_str(htonl(ipaddr), m_ipstr))
	    {
	        _LOG_ERROR("ip to str failed.");
	    }
		m_ipstr[IP_DESC_LEN] = 0;

		if (NULL == engine_ipv4_to_str(htonl(private_ipaddr), m_private_ipstr))
	    {
	        _LOG_ERROR("ip to str failed.");
	    }
		m_private_ipstr[IP_DESC_LEN] = 0;

	#ifdef _WIN32
        m_data_lock = 0;
    #else
        pthread_spin_init(&m_data_lock, 0);
    #endif
	}

	virtual ~CNetObjSet()
	{
		if (m_conn_list.size() != 0)
		{
			_LOG_ERROR("there still has some server leaked when destruct CNetObjSet");
		}

#ifdef _WIN32
        m_data_lock = 0;
#else
        pthread_spin_destroy(&m_data_lock);
#endif
	}

public:
	BOOL is_self(uint32_t pub_ipaddr, uint32_t private_ipaddr);
	BOOL is_self(const char* pub_ipaddr_str, const char* private_ipaddr_str);

	void lock();
	void unlock();
	
	int add_server_obj(int index);
	void del_server_obj(int index);

	int get_server_obj_cnt();

	virtual void print_statistic(FILE *pFd) = 0;
	virtual void aged_netobj() = 0;

public:
    uint32_t m_ipaddr;
	char m_ipstr[IP_DESC_LEN + 1];

	uint32_t m_private_ipaddr;
	char m_private_ipstr[IP_DESC_LEN + 1];

private:
    SRV_INDEX_LIST m_conn_list;

#ifdef _WIN32
	LONG m_data_lock;
#else
	pthread_spinlock_t m_data_lock;
#endif
};

class CClientNetSet : public CNetObjSet
{
public:
	CClientNetSet(uint32_t ipaddr, uint32_t private_ipaddr) : CNetObjSet(ipaddr, private_ipaddr)
	{
	}
	virtual ~CClientNetSet()
	{
	}

public:
	void print_statistic(FILE *pFd);
	void aged_netobj();
};

class CSocksNetSet : public CNetObjSet
{
public:
	CSocksNetSet(uint32_t ipaddr, uint32_t private_ipaddr, CServerCfg *srvCfg) : CNetObjSet(ipaddr, private_ipaddr)
	{
		m_srvCfg = *srvCfg;

		/*上线处理*/
        g_SrvCfgMgr->set_server_online(m_srvCfg.m_sn, m_srvCfg.m_pub_ip, m_srvCfg.m_pri_ip);

        if (is_relay_need_platform())
	    {
			/*通知平台*/
		    if(0 != g_webApi->postServerOnline(g_relaysn, m_srvCfg.m_sn, m_srvCfg.m_pub_ip, m_srvCfg.m_pri_ip, TRUE))
		    {
		        _LOG_WARN("socksserver %s, localip %s failed to post platform online", srvCfg->m_pub_ip, srvCfg->m_pri_ip);
		    }
		}
	}
	virtual ~CSocksNetSet()
	{
		/*下线处理*/
    	g_SrvCfgMgr->set_server_offline(m_srvCfg.m_sn, m_srvCfg.m_pub_ip, m_srvCfg.m_pri_ip);

    	if (is_relay_need_platform())
    	{
	    	/*通知平台*/
		    if(0 != g_webApi->postServerOnline(g_relaysn, m_srvCfg.m_sn, m_srvCfg.m_pub_ip, m_srvCfg.m_pri_ip, FALSE))
		    {
		        _LOG_WARN("socksserver %s, localip %s failed to post platform offline", m_srvCfg.m_pub_ip, m_srvCfg.m_pri_ip);
		    }
		}
	}

public:
	void print_statistic(FILE *pFd);
	void aged_netobj();

	void set_srv_cfg(CServerCfg *srvCfg);
	bool user_authen(const char *username, const char *passwd);
		
	int get_active_socks_server();
	void close_all_socks_server();

private:
    CServerCfg m_srvCfg;
};

#endif

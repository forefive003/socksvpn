#ifndef _CONFIG_WEBAPI_H
#define _CONFIG_WEBAPI_H

#include "CHttpClient.h"
#include "json.h"
#include "engine_ip.h"

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

		m_is_online = false;
		m_is_add_by_packet = false;
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

	void set_online(bool online)
	{
		m_is_online = online;
		_LOG_INFO("set server SN %s, pub %s, pri %s, online %d", m_sn, m_pub_ip, m_pri_ip, online);
	}

	void set_sn_and_ip(char *sn, char *pub_ip, char *pri_ip)
	{
		strncpy(m_sn, sn, MAX_SN_LEN);
		strncpy(m_pub_ip, pub_ip, IP_DESC_LEN);
		strncpy(m_pri_ip, pri_ip, IP_DESC_LEN);
	}

	void set_created_pkt_flag(bool is_add_by_pkt)
	{
		m_is_add_by_packet = is_add_by_pkt;
	}
	
public:
	acct_info_t m_acct_infos[MAX_ACCT_PER_SERVER];
	int m_acct_cnt;

	char m_sn[MAX_SN_LEN];
	char m_pub_ip[IP_DESC_LEN];
	char m_pri_ip[IP_DESC_LEN];

	bool m_is_online; /*是否在线*/
	bool m_is_add_by_packet; /*是否根据报文创建的*/
};

enum
{
	POST_RELAY_ONLINE = 1,
	POST_SERVER_ONLINE = 2,
};

class CWebApi {
public:
	CWebApi(char *url, bool is_debug=false)
	{
		m_url = url;
		if (is_debug)
			m_http_client.setDebug();
	}
	virtual ~CWebApi()
	{

	}

    static CWebApi* instance(char *url);

	int postRelayOnline(char *relaySn, char *relayPasswd, short relayPort);
	int postServerOnline(char *relaySn, char *serverSn, char *pub_ip, char *pri_ip, bool online);
	int getRelayConfig(char *relaySn);
	int getRelayServerIpPort(char *relay_ipstr, short *relay_port);
private:
	std::string m_url;
	CHttpClient  m_http_client;

	int _parseServerList(struct json_object *listObj);
	virtual void addServerCfg(CServerCfg *srvCfg)
	{
		_LOG_ERROR("not should be here parser server cfg.");
		return;
	}
};

extern CWebApi *g_webApi;

#endif

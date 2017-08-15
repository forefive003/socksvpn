#ifndef _CONFIG_WEBAPI_H
#define _CONFIG_WEBAPI_H

#include "CHttpClient.h"
#include "json.h"

enum
{
	POST_RELAY_ONLINE = 1,
	POST_SERVER_ONLINE = 2,
};

class CWebApi {
public:
	CWebApi(char *url)
	{
		m_url = url;
		m_http_client.setDebug();
	}
	virtual ~CWebApi()
	{

	}

    static CWebApi* instance(char *url);

	int postRelayOnline(char *relaySn, char *relayPasswd);
	int postServerOnline(char *serverSn, char *pub_ip, char *pri_ip, bool online);
	int getRelayConfig(char *relaySn);
private:
	std::string m_url;
	CHttpClient  m_http_client;

	int _parseServerList(struct json_object *listObj);
};

extern CWebApi *g_webApi;

#endif

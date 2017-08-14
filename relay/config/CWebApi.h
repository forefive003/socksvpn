#ifndef _CONFIG_WEBAPI_H
#define _CONFIG_WEBAPI_H

#include "CHttpClient.h"

enum
{
	POST_RELAY_ONLINE = 1,
	POST_SERVER_ONLINE = 2,
};

class CWebApi {
public:
	CWebApi(char *url);
	virtual ~CWebApi();

    static CWebApi* instance(char *url);

	int postRelayOnline(char *relaySn, char *relayPasswd);
	int postServerOnline(char *serverSn, char *pub_ip, char *pri_ip, bool online);
	int getRelayConfig(char *relaySn);
private:
	std::string m_url;
	CHttpClient  m_http_client;
};

#endif

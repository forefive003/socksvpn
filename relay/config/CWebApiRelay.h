#ifndef _CONFIG_WEBAPI_RELAY_H
#define _CONFIG_WEBAPI_RELAY_H

#include "CWebApi.h"
#include "CServerCfg.h"

class CWebApiRelay : public CWebApi {
public:
	CWebApiRelay(char *url) : CWebApi(url)
    {
    }
	virtual ~CWebApiRelay()
	{
	}
	virtual void addServerCfg(CServerCfg *srvCfg)
	{
		/*添加服务器配置*/
	    g_SrvCfgMgr->add_server_cfg(srvCfg);
		return;
	}

	static CWebApiRelay* instance(char *url)
	{
		static CWebApiRelay *webApi = NULL;

	    if(webApi == NULL)
	    {
	        webApi = new CWebApiRelay(url);
	    }
	    return webApi;
	}
};

#endif

#include <netinet/tcp.h>
#include "json.h"
#include "bits.h"
#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "socks_relay.h"
#include "CWebApi.h"
#include "CNetAccept.h"
#include "CNetRecv.h"
#include "common_def.h"
#include "CConfigServer.h"
#include "CSocksMem.h"
#include "CSocksSrvMgr.h"
#include "socks_relay.h"
#include "CServerCfg.h"


CWebApi *g_webApi = NULL;

CWebApi* CWebApi::instance(char *url)
{
    static CWebApi *webApi = NULL;

    if(webApi == NULL)
    {
        webApi = new CWebApi(url);
    }
    return webApi;
}

int CWebApi::postRelayOnline(char *relaySn, char *relayPasswd)
{
	char req[256] = {0};
	snprintf(req, 256, "{\"cmd\":1,\"relaysn\":\"%s\",\"password\":\"%s\"}", relaySn, relayPasswd);
	std::string reqStr = req;
	std::string responseStr;

	int ret = m_http_client.Post(m_url, reqStr, responseStr);
	if (0 != ret)
	{
		_LOG_ERROR("http post failed, url %s, req %s", m_url.c_str(), req);
		return ret;
	}

	/*没有回应内容*/
	if (responseStr.length() == 0)
	{
		_LOG_ERROR("len of response is zero for url %s.", m_url.c_str());
		return -1;
	}

	struct json_object *new_obj = NULL;
	struct json_object *res_obj = NULL;
	const char* res_str = NULL;

	const char *response = responseStr.c_str();
	_LOG_INFO("get response %s when post relay server online", response);

	new_obj = json_tokener_parse(response);
	if( is_error(new_obj))
	{
		_LOG_ERROR("recv str not json : %s", response);
		return -1;
	}

	res_obj = json_object_object_get(new_obj, "code");
	if (NULL == res_obj)
	{
		json_object_put(new_obj);
		_LOG_ERROR("response str has no code field : %s", response);
		return -1;
	}
	res_str = json_object_get_string(res_obj);
	if (atoi(res_str) == 1)
	{
		json_object_put(new_obj);
		_LOG_ERROR("post relay server online failed");
		return -1;
	}

	json_object_put(new_obj);
	_LOG_INFO("post relay server online success");
	return 0;
}

int CWebApi::postServerOnline(char *serverSn, char *pub_ip, char *pri_ip, bool online)
{
	char req[256] = {0};
	snprintf(req, 256, "{\"cmd\":2,\"sn\":\"%s\",\"pub-ip\":\"%s\",\"pri-ip\":\"%s\",\"online\":\"%d\"}", 
		serverSn, pub_ip, pri_ip, online);
	std::string reqStr = req;
	std::string responseStr;

	int ret = m_http_client.Post(m_url, reqStr, responseStr);
	if (0 != ret)
	{
		_LOG_ERROR("http post failed, url %s, req %s", m_url.c_str(), req);
		return ret;
	}

	/*没有回应内容*/
	if (responseStr.length() == 0)
	{
		_LOG_ERROR("len of response is zero for url %s.", m_url.c_str());
		return -1;
	}

	struct json_object *new_obj = NULL;
	struct json_object *res_obj = NULL;
	const char* res_str = NULL;

	const char *response = responseStr.c_str();
	_LOG_INFO("get response %s when post server online", response);

	new_obj = json_tokener_parse(response);
	if( is_error(new_obj))
	{
		_LOG_ERROR("recv str not json : %s", response);
		return -1;
	}

	res_obj = json_object_object_get(new_obj, "code");
	if (NULL == res_obj)
	{
		json_object_put(new_obj);
		_LOG_ERROR("response str has no code field : %s", response);
		return -1;
	}
	res_str = json_object_get_string(res_obj);
	if (atoi(res_str) == 1)
	{
		json_object_put(new_obj);
		_LOG_ERROR("post server online failed");
		return -1;
	}

	json_object_put(new_obj);
	_LOG_INFO("post server online success");
	return 0;
}

/*
[
	{
		"sn":"AABBCCDD002233",
		"pub-ip":"202.1.1.2", 
		"pri-ip":"192.168.1.1",
		"users":[
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
				]

    },
    {
		"sn":"AABBCCDD002244",
		"pub-ip":"202.1.1.3", 
		"pri-ip":"192.168.0.1",
		"users":[
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
					｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
				]
    },
	......
]
*/
int CWebApi::_parseServerList(struct json_object *listObj)
{
	if (false == json_object_is_type(listObj, json_type_array))
    {
        _LOG_ERROR("list param not a json array when get.");
        return -1;
    }

    uint32_t i = 0;
    uint32_t arr_len = json_object_array_length(listObj);
    for (i = 0; i < arr_len; i++)
    {
        struct json_object *arrObj = json_object_array_get_idx(listObj, i);
        CServerCfg srvCfg;

	    /*sn*/
	    struct json_object *tmpObj = json_object_object_get(arrObj, "sn");
	    if (tmpObj == NULL)
	    {
	        _LOG_ERROR("recv invalid json str, server sn failed.");
	        return -1;
	    }
	    const char* tmpStr = json_object_get_string(tmpObj);
	    if (NULL == tmpStr)
	    {
	        _LOG_ERROR("recv invalid json str, server sn value failed.");
	        return -1;
	    }
	    strncpy(srvCfg.m_sn, tmpStr, MAX_SN_LEN);
	    
	    /*pub ip*/
	    tmpObj = json_object_object_get(arrObj, "pub-ip");
	    if (tmpObj == NULL)
	    {
	        _LOG_ERROR("recv invalid json str, pub-ip failed.");
	        return -1;
	    }
	    tmpStr = json_object_get_string(tmpObj);
	    if (NULL == tmpStr)
	    {
	        _LOG_ERROR("recv invalid json str, pub-ip value failed.");
	        return -1;
	    }
	    strncpy(srvCfg.m_pub_ip, tmpStr, IP_DESC_LEN);

	    /*pri-ip*/
	    tmpObj = json_object_object_get(arrObj, "pri-ip");
	    if (tmpObj == NULL)
	    {
	        _LOG_ERROR("recv invalid json str, pri-ip failed.");
	        return -1;
	    }
	    tmpStr = json_object_get_string(tmpObj);
	    if (NULL == tmpStr)
	    {
	        _LOG_ERROR("recv invalid json str, pri-ip value failed.");
	        return -1;
	    }
	    strncpy(srvCfg.m_pri_ip, tmpStr, IP_DESC_LEN);

	    /*    
	    "users":[
	                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
	                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
	                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
	                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
	                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
	            ]
	    */
	    /*users*/
	    tmpObj = json_object_object_get(arrObj, "users");
	    if (tmpObj == NULL)
	    {
	        _LOG_ERROR("recv invalid json str, users failed.");
	        return -1;
	    }
	    if (false == json_object_is_type(tmpObj, json_type_array))
	    {
	        _LOG_ERROR("users param not a json array when set.");
	        return -1;
	    }

	    uint32_t ii = 0;
	    uint32_t arr_len1 = json_object_array_length(tmpObj);
	    for (ii = 0; ii < arr_len1; ii++)
	    {
	        struct json_object *arrObj1 = json_object_array_get_idx(tmpObj, ii);

	        /*username*/
	        struct json_object *tmpObj1 = json_object_object_get(arrObj1, "username");
	        if (tmpObj1 == NULL)
	        {
	            _LOG_ERROR("recv invalid json str, username failed.");
	            return -1;
	        }
	        tmpStr = json_object_get_string(tmpObj1);
	        if (NULL == tmpStr)
	        {
	            _LOG_ERROR("username value failed.");
	            return -1;
	        }
	        strncpy(srvCfg.m_acct_infos[srvCfg.m_acct_cnt].username, tmpStr, MAX_USERNAME_LEN);

	        /*passwd*/
	        tmpObj1 = json_object_object_get(arrObj1, "passwd");
	        if (tmpObj1 == NULL)
	        {
	            _LOG_ERROR("recv invalid json str, passwd failed.");
	            return -1;
	        }
	        tmpStr = json_object_get_string(tmpObj1);
	        if (NULL == tmpStr)
	        {
	            _LOG_ERROR("passwd value failed.");
	            return -1;
	        }
	        strncpy(srvCfg.m_acct_infos[srvCfg.m_acct_cnt].passwd, tmpStr, MAX_PASSWD_LEN);

	        /*enabled*/
	        tmpObj1 = json_object_object_get(arrObj1, "enabled");
	        if (tmpObj1 == NULL)
	        {
	            _LOG_ERROR("recv invalid json str, enabled failed.");
	            return -1;
	        }
	        tmpStr = json_object_get_string(tmpObj1);
	        if (NULL == tmpStr)
	        {
	            _LOG_ERROR("enabled value failed.");
	            return -1;
	        }
	        if (0 == atoi(tmpStr))
	        {
	            srvCfg.m_acct_infos[srvCfg.m_acct_cnt].enabled = true;
	        }
	        else
	        {
	            srvCfg.m_acct_infos[srvCfg.m_acct_cnt].enabled = false;
	        }

	        /*个数加一*/
	        srvCfg.m_acct_cnt++;
	    }

	    /*添加服务器配置*/
	    g_SrvCfgMgr->add_server_cfg(&srvCfg);
	}

	return 0;
}

int CWebApi::getRelayConfig(char *relaySn)
{
	char urlreq[256] = {0};
	snprintf(urlreq, 256, "%s?relaysn=%s", m_url.c_str(), relaySn);
	std::string urlStr = urlreq;
	std::string responseStr;

	int ret = m_http_client.Get(urlStr, responseStr);
	if (0 != ret)
	{
		_LOG_ERROR("http get failed, url %s", m_url.c_str());
		return ret;
	}

	/*没有回应内容*/
	if (responseStr.length() == 0)
	{
		_LOG_ERROR("len of response is zero for url %s.", m_url.c_str());
		return -1;
	}

	struct json_object *new_obj = NULL;
	struct json_object *res_obj = NULL;
	const char* res_str = NULL;

	const char *response = responseStr.c_str();
	_LOG_INFO("get response %s when post server online", response);

	new_obj = json_tokener_parse(response);
	if( is_error(new_obj))
	{
		_LOG_ERROR("recv str not json : %s", response);
		return -1;
	}

	res_obj = json_object_object_get(new_obj, "code");
	if (NULL == res_obj)
	{
		json_object_put(new_obj);
		_LOG_ERROR("response str has no code field : %s", response);
		return -1;
	}
	res_str = json_object_get_string(res_obj);
	if (atoi(res_str) == 1)
	{
		json_object_put(new_obj);
		_LOG_ERROR("post server online failed");
		return -1;
	}

	res_obj = json_object_object_get(new_obj, "list");
	if (NULL == res_obj)
	{
		json_object_put(new_obj);
		_LOG_ERROR("response str has no list field : %s", response);
		return -1;
	}

	ret = _parseServerList(res_obj);
	if(ret != 0)
	{
		_LOG_ERROR("get server config failed");
	}
	else
	{
		_LOG_INFO("get server config success");
	}

	json_object_put(new_obj);
	return ret;
}


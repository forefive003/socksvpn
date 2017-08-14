#include <netinet/tcp.h>
#include "json.h"
#include "bits.h"
#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "socks_relay.h"
#include "CWebApi.h"

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

	int ret = m_http_client.Post(m_url, reqStr, &responseStr);
	if (0 != ret)
	{
		_LOG_ERROR("http post failed, url %s, req %s", url, req);
		return ret;
	}

	/*没有回应内容*/
	if (responseStr.length() == 0)
	{
		_LOG_ERROR("len of response is zero for url %s.", url);
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

	int ret = m_http_client.Post(m_url, reqStr, &responseStr);
	if (0 != ret)
	{
		_LOG_ERROR("http post failed, url %s, req %s", url, req);
		return ret;
	}

	/*没有回应内容*/
	if (responseStr.length() == 0)
	{
		_LOG_ERROR("len of response is zero for url %s.", url);
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

int CWebApi::getRelayConfig(char *relaySn)
{
	char req[256] = {0};
	snprintf(req, 256, "{\"relaysn\":\"%s\"}", relaySn);
	std::string reqStr = req;
	std::string responseStr;

	int ret = m_http_client.Post(m_url, reqStr, &responseStr);
	if (0 != ret)
	{
		_LOG_ERROR("http post failed, url %s, req %s", url, req);
		return ret;
	}

	/*没有回应内容*/
	if (responseStr.length() == 0)
	{
		_LOG_ERROR("len of response is zero for url %s.", url);
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


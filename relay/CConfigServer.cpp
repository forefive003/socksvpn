#include <netinet/tcp.h>
#include "json.h"
#include "bits.h"
#include "commtype.h"
#include "logproc.h"
#include "CNetAccept.h"
#include "CNetRecv.h"
#include "common_def.h"
#include "CConfigServer.h"
#include "CSocksMem.h"
#include "CSocksSrvMgr.h"

CConfigAccept *g_ConfigServ = NULL;

int CConfigAccept::accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port)
{
    int keepalive = 1; // 开启keepalive属性
    int keepidle = 10; // 如该连接在60秒内没有任何数据往来,则进行探测
    int keepinterval = 5; // 探测时发包的时间间隔为5 秒
    int keepcount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    setsockopt(conn_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
    setsockopt(conn_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
    setsockopt(conn_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
    setsockopt(conn_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));

    CConfigSrv *cfgSrv = new CConfigSrv(client_ip, client_port, conn_fd);
    cfgSrv->init_async_write_resource(socks_malloc, socks_free);
    if (0 != cfgSrv->init())
    {
        delete cfgSrv;
        return -1;
    }

    return 0;
}

/*
request: {"obj":"all"}
response: "[{"pub-ip":"202.1.1.1", "pri-ip":"192.168.1.1", "username":"hahaha", "passwd":"xxxx", "enabled":1}, 
            {},
            {}]"
*/
int CConfigSrv::_cfg_get_sock_srv_info(struct json_object *paramObj, char *resp_buf, int buf_len)
{
    _LOG_INFO("get all server info");
    return g_SocksSrvMgr->output_socks_server(resp_buf, buf_len);
}

/*
request: {["pub-ip":"202.1.1.1", "pri-ip":"192.168.1.1", "username":"hahaha", "passwd":"xxxx", "enabled":1], 
            [],
            [] }
response: ""
*/
int CConfigSrv::_cfg_set_sock_srv_info(struct json_object *paramObj, char *resp_buf, int buf_len)
{
    struct json_object *tmpObj = NULL;
    struct json_object *arrObj = NULL;

    const char* pubIpStr = NULL;
    const char* priIpStr = NULL;
    const char* usernameStr = NULL;
    const char* passwdStr = NULL;
    int enabled = 0;
    bool has_enabled_opt = false;

    uint32_t i = 0;
    uint32_t arr_len = json_object_array_length(paramObj);
    if (false == json_object_is_type(paramObj, json_type_array))
    {
        _LOG_ERROR("sock srv param not a json array when set.");
        return -1;
    }

    for (i = 0; i < arr_len; i++)
    {
        arrObj = json_object_array_get_idx(paramObj, i);

        /*pub-ip*/
        tmpObj = json_object_object_get(arrObj, "pub-ip");
        if (tmpObj == NULL)
        {
            _LOG_ERROR("no pub-ip found.");
            return -1;
        }
        pubIpStr = json_object_get_string(tmpObj);
        if (NULL == pubIpStr)
        {
            _LOG_ERROR("no pub-ip found.");
            return -1;
        }
        
        /*pri-ip*/
        tmpObj = json_object_object_get(arrObj, "pri-ip");
        if (tmpObj == NULL)
        {
            _LOG_ERROR("no pri-ip found.");
            return -1;
        }
        priIpStr = json_object_get_string(tmpObj);
        if (NULL == priIpStr)
        {
            _LOG_ERROR("no pri-ip found.");
            return -1;
        }

        /*username*/
        tmpObj = json_object_object_get(arrObj, "username");
        if (tmpObj != NULL)
        {
            usernameStr = json_object_get_string(tmpObj);
            if (NULL == usernameStr)
            {
                _LOG_ERROR("no username found.");
                return -1;
            }
        }
        

        /*passwd*/
        tmpObj = json_object_object_get(arrObj, "passwd");
        if (tmpObj != NULL)
        {
            passwdStr = json_object_get_string(tmpObj);
            if (NULL == passwdStr)
            {
                _LOG_ERROR("no passwd found.");
                return -1;
            }
        }
        

        /*enabled*/
        tmpObj = json_object_object_get(arrObj, "enabled");
        if (tmpObj != NULL)
        {
            has_enabled_opt = true;
            enabled = json_object_get_int(tmpObj);
        }
        
        
        CSocksSrv *socksSrv = NULL;
        g_SocksSrvMgr->lock();
        socksSrv = g_SocksSrvMgr->get_socks_server_by_innnerip(pubIpStr, priIpStr);
        if (NULL == socksSrv)
        {
            g_SocksSrvMgr->unlock();
            _LOG_ERROR("socksserver %s(%s) not exist", pubIpStr, priIpStr);
            return -1;
        }

        if (usernameStr != NULL && passwdStr != NULL)
        {
            socksSrv->set_user_passwd(usernameStr, passwdStr);
        }

        if (has_enabled_opt)
        {
            socksSrv->set_enabled((enabled == 0)?false:true);
        }
        g_SocksSrvMgr->unlock();
    }

    return 0;
}

/*
request: "params":{"level":"debug|info|warn"}
response: "params":""
*/
int CConfigSrv::_cfg_set_debug_level(struct json_object *paramObj, char *resp_buf, int buf_len)
{
    struct json_object *tmpObj = NULL;
    const char* tmpStr = NULL;

    /*type*/
    tmpObj = json_object_object_get(paramObj, "level");
    if (tmpObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, level failed.");
        goto errRet;
    }
    tmpStr = json_object_get_string(tmpObj);
    if (NULL == tmpStr)
    {
        _LOG_ERROR("recv invalid json str, level value failed.");
        goto errRet;
    }

    _LOG_INFO("set debug level: %s", tmpStr);

    if (strncmp(tmpStr, "debug", strlen("debug")) == 0)
    {
        logger_set_level(L_DEBUG);
    }
    else if (strncmp(tmpStr, "info", strlen("info")) == 0)
    {
        logger_set_level(L_INFO);
    }
    else if (strncmp(tmpStr, "warn", strlen("warn")) == 0)
    {
        logger_set_level(L_WARN);
    }
    else
    {
        _LOG_ERROR("recv invalid json str, level value failed.");
        goto errRet;
    }
    return 0;

errRet:
    return -1;
}

int CConfigSrv::request_handle(char *buffer, int buf_len, char *resp_buf, int resp_buf_len)
{
    struct json_object *new_obj = NULL;

    struct json_object *typeObj = NULL;
    const char* typeStr = NULL;
    struct json_object *targetObj = NULL;
    const char* targetStr = NULL;
    struct json_object *paramObj = NULL;

    int ret = 0;

    new_obj = json_tokener_parse((const char*)buffer);
    if( is_error(new_obj))
    {
        _LOG_ERROR("recv invalid json str %s.", buffer);
        return -1;
    }

    /*type*/
    typeObj = json_object_object_get(new_obj, "operation");
    if (typeObj == NULL)
    {
        _LOG_ERROR("recv invalid json str %s, operation failed.", buffer);
        goto errRet;
    }
    typeStr = json_object_get_string(typeObj);
    if (NULL == typeStr)
    {
        _LOG_ERROR("recv invalid json str %s, operation value failed.", buffer);
        goto errRet;
    }

    /*target*/
    targetObj = json_object_object_get(new_obj, "target");
    if (targetObj == NULL)
    {
        _LOG_ERROR("recv invalid json str %s, target failed.", buffer);
        goto errRet;
    }
    targetStr = json_object_get_string(targetObj);
    if (NULL == targetStr)
    {
        _LOG_ERROR("recv invalid json str %s, target value failed.", buffer);
        goto errRet;
    }

    /*param*/
    paramObj = json_object_object_get(new_obj, "param");
    if (paramObj == NULL)
    {
        _LOG_ERROR("recv invalid json str %s, param failed.", buffer);
        goto errRet;
    }

    if (strncmp(typeStr, "get", strlen("get")) == 0)
    {
        /*target*/
        if (strncmp(targetStr, "socks-srv", strlen("socks-srv")) == 0)
        {
            ///获取所有socks-srv的信息
            ret = _cfg_get_sock_srv_info(paramObj, resp_buf, resp_buf_len);
        }
        else
        {
            _LOG_ERROR("recv invalid json str %s, target failed.", buffer);
            goto errRet;
        }
    }
    else if (strncmp(typeStr, "set", strlen("set")) == 0)
    {
        /*target*/
        if (strncmp(targetStr, "socks-srv", strlen("socks-srv")) == 0)
        {
            ret = _cfg_set_sock_srv_info(paramObj, resp_buf, resp_buf_len);
        }
        else if (strncmp(targetStr, "debug", strlen("debug")) == 0)
        {
            ret = _cfg_set_debug_level(paramObj, resp_buf, resp_buf_len);
        }
        else
        {
            _LOG_ERROR("recv invalid json str %s, target failed.", buffer);
            goto errRet;
        }
    }
    else
    {
        _LOG_ERROR("recv invalid json str %s, invalid type %s.", buffer, typeStr);
        goto errRet;
    }

    json_object_put(new_obj);
    return ret;

errRet:
    json_object_put(new_obj);
    return -1;
}

int CConfigSrv::recv_handle(char *buf, int buf_len)
{
    /*it has data not copy yet, copy it now*/
    int spare_len = sizeof(m_recv_buf) - m_recv_len;
    if (buf_len > spare_len)
    {
        _LOG_ERROR("cur recv len %d, but spare_len %u too less to fill", m_recv_len, buf_len);
        return -1;
    }
    else
    {
        memcpy(&m_recv_buf[m_recv_len], buf, buf_len);
        m_recv_len += buf_len;
    }

    if (m_recv_buf[m_recv_len-1] != '\0')
    {
        _LOG_DEBUG("cur recv len %d, wait all msg come here", m_recv_len);
        return 0;
    }

    _LOG_INFO("recv config msg: %s, len %u.", m_recv_buf, m_recv_len);

    /*处理json*/
    char header[256] = {0};

    int body_len = request_handle(m_recv_buf, m_recv_len, &m_resp_buf[0], MAX_RESP_MSG_LEN);
    if (-1 == body_len)
    {
        snprintf(header, MAX_RESP_MSG_LEN, "{\"result\":\"failed\", \"param\":\"\"}");
        _LOG_INFO("response msg: %s, len %d", header, strlen(header));

        /*回应*/
        this->send_data(header, strlen(header)+1);
    }
    else
    {
        if (0 == body_len)
        {
            snprintf(header, MAX_RESP_MSG_LEN, "{\"result\":\"success\", \"param\":\"\"}");
            _LOG_INFO("response msg: %s, len %d", header, strlen(header));

            /*回应*/
            this->send_data(header, strlen(header)+1);
        }
        else
        {
            snprintf(header, MAX_RESP_MSG_LEN, "{\"result\":\"success\", \"param\":");
            m_resp_buf[ body_len] = '}';
            m_resp_buf[ body_len + 1] = 0;

            _LOG_INFO("response msg: %s%s, len %d", header, m_resp_buf, strlen(header) + body_len + 2);

            /*回应*/
            this->send_data(header, strlen(header));
            this->send_data(m_resp_buf, body_len + 2);
        }
    }

    m_recv_len = 0;
    return 0;
}

void CConfigSrv::free_handle()
{
    delete this;
}

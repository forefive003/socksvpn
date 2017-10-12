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
#include "socks_relay.h"
#include "CServerCfg.h"

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
    //cfgSrv->init_async_write_resource(socks_malloc, socks_free);
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
int CConfigSrv::_get_server_cfg(struct json_object *paramObj, char *resp_buf, int buf_len)
{
    _LOG_INFO("get all server info");
    return 0;
    //return g_SocksSrvMgr->output_socks_server(resp_buf, buf_len);
}

/*
{
    "cmd":2,
    "relaysn":"sakjgukagklnknk",//relay_server需要验证其合法性
    "password":"aaaa",//relay_server需要验证其合法性
    "sn":"AABBCCDD002233bb"//socks_server sn
}
*/
int CConfigSrv::_del_server_cfg(struct json_object *paramObj, char *resp_buf, int buf_len)
{
    struct json_object *relaysnObj = NULL;
    const char* relaysnStr = NULL;
    struct json_object *passwordObj = NULL;
    const char* passwordStr = NULL;
    struct json_object *serverSnObj = NULL;
    const char* serverSnStr = NULL;

    /*relaysn*/
    relaysnObj = json_object_object_get(paramObj, "relaysn");
    if (relaysnObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, relaysn failed.");
        return -1;
    }
    relaysnStr = json_object_get_string(relaysnObj);
    if (NULL == relaysnStr)
    {
        _LOG_ERROR("recv invalid json str, relaysn value failed.");
        return -1;
    }
    /*检查sn是否正确*/
    if (strncmp(relaysnStr, g_relaysn, MAX_SN_LEN) != 0)
    {
        _LOG_ERROR("relaysn %s not wanted, should be %s.", relaysnStr, g_relaysn);
        return -1;
    }

    /*password*/
    passwordObj = json_object_object_get(paramObj, "password");
    if (passwordObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, password failed.");
        return -1;
    }
    passwordStr = json_object_get_string(passwordObj);
    if (NULL == passwordStr)
    {
        _LOG_ERROR("recv invalid json str, password value failed.");
        return -1;
    }
    /*检查passwd是否正确*/
    if (strncmp(passwordStr, g_relay_passwd, MAX_PASSWD_LEN) != 0)
    {
        _LOG_ERROR("relay passwd %s not wanted, should be %s.", passwordStr, g_relay_passwd);
        return -1;
    }

    /*body*/
    serverSnObj = json_object_object_get(paramObj, "sn");
    if (serverSnObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, server sn failed.");
        return -1;
    }
    serverSnStr =  json_object_get_string(serverSnObj);
    if (NULL == serverSnStr)
    {
        _LOG_ERROR("recv invalid json str, server sn value failed.");
        return -1;
    }

    /*根据sn获取配置*/
    g_SrvCfgMgr->del_server_cfg((char*)serverSnStr);
    return 0;
}

/*
"relaysn":"sakjgukagklnknk",//relay_server需要验证其合法性
"password":"aaaa",//relay_server需要验证其合法性
"body":{
            "sn":"AABBCCDD002233bb",//socks_server序列号,通过运行时参数指定
            "pub-ip":"202.1.1.2", //socks_server外网ip
            "pri-ip":"192.168.1.1",//socks_server内网ip
            "users":[
                        ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                        ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                        ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                        ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                        ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                    ]
        }
*/
int CConfigSrv::_set_server_cfg(struct json_object *paramObj, char *resp_buf, int buf_len)
{    
    struct json_object *relaysnObj = NULL;
    const char* relaysnStr = NULL;
    struct json_object *passwordObj = NULL;
    const char* passwordStr = NULL;
    struct json_object *bodyObj = NULL;

    /*relaysn*/
    relaysnObj = json_object_object_get(paramObj, "relaysn");
    if (relaysnObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, relaysn failed.");
        return -1;
    }
    relaysnStr = json_object_get_string(relaysnObj);
    if (NULL == relaysnStr)
    {
        _LOG_ERROR("recv invalid json str, relaysn value failed.");
        return -1;
    }
    /*检查sn是否正确*/
    if (strncmp(relaysnStr, g_relaysn, MAX_SN_LEN) != 0)
    {
        _LOG_ERROR("relaysn %s not wanted, should be %s.", relaysnStr, g_relaysn);
        return -1;
    }

    /*password*/
    passwordObj = json_object_object_get(paramObj, "password");
    if (passwordObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, password failed.");
        return -1;
    }
    passwordStr = json_object_get_string(passwordObj);
    if (NULL == passwordStr)
    {
        _LOG_ERROR("recv invalid json str, password value failed.");
        return -1;
    }
    /*检查passwd是否正确*/
    if (strncmp(passwordStr, g_relay_passwd, MAX_PASSWD_LEN) != 0)
    {
        _LOG_ERROR("relay passwd %s not wanted, should be %s.", passwordStr, g_relay_passwd);
        return -1;
    }

    /*body*/
    bodyObj = json_object_object_get(paramObj, "body");
    if (bodyObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, body failed.");
        return -1;
    }

    /*
    "sn":"AABBCCDD002233bb",//socks_server序列号,通过运行时参数指定
    "pub-ip":"202.1.1.2", //socks_server外网ip
    "pri-ip":"192.168.1.1",//socks_server内网ip
    "users":[
                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
            ]
    */
    struct json_object *tmpObj = NULL;
    const char* tmpStr = NULL;
    CServerCfg srvCfg;

    /*sn*/
    tmpObj = json_object_object_get(bodyObj, "sn");
    if (tmpObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, server sn failed.");
        return -1;
    }
    tmpStr = json_object_get_string(tmpObj);
    if (NULL == tmpStr)
    {
        _LOG_ERROR("recv invalid json str, server sn value failed.");
        return -1;
    }
    memset(srvCfg.m_sn, 0, MAX_SN_LEN);
    strncpy(srvCfg.m_sn, tmpStr, MAX_SN_LEN);
    
    /*pub ip*/
    tmpObj = json_object_object_get(bodyObj, "pub-ip");
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
    memset(srvCfg.m_pub_ip, 0, IP_DESC_LEN);
    strncpy(srvCfg.m_pub_ip, tmpStr, IP_DESC_LEN);

    /*pri-ip*/
    tmpObj = json_object_object_get(bodyObj, "pri-ip");
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
    memset(srvCfg.m_pri_ip, 0, IP_DESC_LEN);
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
    tmpObj = json_object_object_get(bodyObj, "users");
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

    srvCfg.m_acct_cnt = 0;
    memset(srvCfg.m_acct_infos, 0, sizeof(srvCfg.m_acct_infos));

    uint32_t i = 0;
    uint32_t arr_len = json_object_array_length(tmpObj);
    for (i = 0; i < arr_len; i++)
    {
        struct json_object *arrObj = json_object_array_get_idx(tmpObj, i);

        /*username*/
        struct json_object *tmpObj1 = json_object_object_get(arrObj, "username");
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
        tmpObj1 = json_object_object_get(arrObj, "passwd");
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
        tmpObj1 = json_object_object_get(arrObj, "enabled");
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
            srvCfg.m_acct_infos[srvCfg.m_acct_cnt].enabled = false;
        }
        else
        {
            srvCfg.m_acct_infos[srvCfg.m_acct_cnt].enabled = true;
        }

        /*个数加一*/
        srvCfg.m_acct_cnt++;
    }

    g_SrvCfgMgr->add_server_cfg(&srvCfg);
    return 0;
}

/*
request: "level":"debug|info|warn"
response: "params":""
*/
int CConfigSrv::_set_debug_level(struct json_object *paramObj, char *resp_buf, int buf_len)
{
    struct json_object *tmpObj = NULL;
    const char* tmpStr = NULL;

    /*type*/
    tmpObj = json_object_object_get(paramObj, "level");
    if (tmpObj == NULL)
    {
        _LOG_ERROR("recv invalid json str, level failed.");
        return -1;
    }
    tmpStr = json_object_get_string(tmpObj);
    if (NULL == tmpStr)
    {
        _LOG_ERROR("recv invalid json str, level value failed.");
        return -1;
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
        return -1;
    }
    return 0;
}

/*
{
    "relaysn":"sakjgukagklnknk",//relay_server需要验证其合法性
    "password":"aaaa",//relay_server需要验证其合法性
    "body":{
                "sn":"AABBCCDD002233bb",//socks_server序列号,通过运行时参数指定
                "pub-ip":"202.1.1.2", //socks_server外网ip
                "pri-ip":"192.168.1.1",//socks_server内网ip
                "users":[
                            ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                            ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                            ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                            ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                            ｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
                        ]
            }
}
或
{
    "type":"set-debug",
    "level":"debug|info|warn"
}
或
{
    "type":"get-server-cfg"
}
*/
int CConfigSrv::request_handle(char *buffer, int buf_len, char *resp_buf, int resp_buf_len)
{
    struct json_object *new_obj = NULL;
    struct json_object *tmpObj = NULL;
    const char *tmpStr = NULL;
    int ret = 0;

    new_obj = json_tokener_parse((const char*)buffer);
    if( is_error(new_obj))
    {
        _LOG_ERROR("recv invalid json str %s.", buffer);
        return -1;
    }

    /*type*/
    tmpObj = json_object_object_get(new_obj, "type");
    if (tmpObj == NULL)
    {
        tmpObj = json_object_object_get(new_obj, "cmd");
        if (NULL == tmpObj)
        {
            _LOG_ERROR("recv invalid json str, no cmd.");
            json_object_put(new_obj);
            return -1;
        }
        tmpStr = json_object_get_string(tmpObj);

        if (atoi(tmpStr) == 1)
        {
            ret = this->_set_server_cfg(new_obj, resp_buf, resp_buf_len);
        }
        else
        {
            ret = this->_del_server_cfg(new_obj, resp_buf, resp_buf_len);
        }
    }
    else
    {
        tmpStr = json_object_get_string(tmpObj);
        if (NULL == tmpStr)
        {
            _LOG_ERROR("recv invalid json str, type value failed.");
            json_object_put(new_obj);
            return -1;
        }

        if (strncmp(tmpStr, "set-debug", strlen("set-debug")) == 0)
        {
            ret = this->_set_debug_level(new_obj, resp_buf, resp_buf_len);
        }
        else if (strncmp(tmpStr, "get-server-cfg", strlen("get-server-cfg")) == 0)
        {
            ret = this->_get_server_cfg(new_obj, resp_buf, resp_buf_len);
        }
        else
        {
            ret = this->_set_server_cfg(new_obj, resp_buf, resp_buf_len);
        }
    }

    json_object_put(new_obj);
    return ret;
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
        snprintf(header, MAX_RESP_MSG_LEN, "{\"code\":1}");
        _LOG_INFO("response msg: %s, len %d", header, strlen(header));

        /*回应*/
        this->send_data(header, strlen(header)+1);
    }
    else
    {
        if (0 == body_len)
        {
            snprintf(header, MAX_RESP_MSG_LEN, "{\"code\":0}");
            _LOG_INFO("response msg: %s, len %d", header, strlen(header));

            /*回应*/
            this->send_data(header, strlen(header)+1);
        }
        else
        {
            snprintf(header, MAX_RESP_MSG_LEN, "{\"code\":0, \"param\":");
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

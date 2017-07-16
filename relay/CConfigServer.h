#ifndef _RELAY_CONFIG_SERVER_H
#define _RELAY_CONFIG_SERVER_H

#include "json.h"

#define MAX_REQ_MSG_LEN  1048576 //1024 * 1024
#define MAX_RESP_MSG_LEN  1048576

class CConfigAccept : public CNetAccept
{
public:   
    CConfigAccept(uint16_t port) : CNetAccept(port)
    {
        _LOG_INFO("construct config server on port %u", m_listen_port);
    }
    virtual ~CConfigAccept()
    {
        _LOG_INFO("destruct config server on port %u", m_listen_port);
    }

public:
    static CConfigAccept* instance(uint16_t port)
    {
        static CConfigAccept *configSrv = NULL;

        if(configSrv == NULL)
        {
            configSrv = new CConfigAccept(port);
        }
        return configSrv;
    }

private:
    int accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port);
};


class CConfigSrv : public CNetRecv
{
public:
    CConfigSrv(uint32_t ipaddr, uint16_t port, int fd) : CNetRecv(ipaddr, port, fd)
    {

    }

    virtual ~CConfigSrv()
    {
        _LOG_INFO("destruct configClient, %s:%u, fd %d", m_ipstr, m_port, m_fd);
    }

private:
    int _cfg_get_sock_srv_info(struct json_object *reqParamObj, char *resp_buf, int buf_len);
    int _cfg_set_sock_srv_info(struct json_object *reqParamObj, char *resp_buf, int buf_len);
    int _cfg_set_debug_level(struct json_object *reqParamObj, char *resp_buf, int buf_len);

    int request_handle(char *buffer, int buf_len, char *resp_buf, int resp_buf_len);

    int recv_handle(char *buf, int buf_len);
    void free_handle();

private:
    char m_recv_buf[MAX_REQ_MSG_LEN];
    int m_recv_len;

    char m_resp_buf[MAX_RESP_MSG_LEN];
};

extern CConfigAccept *g_ConfigServ;

#endif

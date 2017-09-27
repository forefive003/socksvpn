#ifndef _BASE_OBJ_H
#define _BASE_OBJ_H

#include "CNetRecv.h"

class CBaseConnection;

class CBaseObj: public CNetRecv
{
public:
    CBaseObj(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *conn);
    virtual ~CBaseObj();

public:
    virtual BOOL is_self(uint32_t ipaddr, uint16_t port);
    virtual int send_data_msg(char *buf, int buf_len);
    virtual uint32_t get_ipaddr();
    virtual char* get_ipstr();
    virtual uint16_t get_port();
    virtual int get_fd();

public:
    CBaseConnection *m_owner_conn;
};

class CBaseClient : public CBaseObj
{
public:
    CBaseClient(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *conn);
    virtual ~CBaseClient();

public:
    void free_handle();

public:
    BOOL is_self(uint32_t ipaddr, uint16_t port, uint32_t inner_ipaddr, uint16_t inner_port);
    void set_inner_info(uint32_t inner_ipaddr, uint16_t inner_port);
    uint32_t get_inner_ipaddr();
    uint16_t get_inner_port();
    virtual int send_remote_close_msg()
    {
        return 0;
    }
public:
    uint32_t m_inner_ipaddr;
    uint16_t m_inner_port;

    char m_inner_ipstr[HOST_IP_LEN + 1];
};

class CBaseRemote : public CBaseObj
{
public:
    CBaseRemote(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *conn);
    virtual ~CBaseRemote();

    virtual int send_client_close_msg()
    {
        return 0;
    }
public:
    void free_handle();
};

#endif

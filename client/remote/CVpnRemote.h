#ifndef _VPN_REMOTE_H
#define _VPN_REMOTE_H

#include "CRemoteServerPool.h"

class CVpnRemote : public CBaseRemote
{
public:
    CVpnRemote(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner, int srv_index) : CBaseRemote(ipaddr, port, fd, owner)
    {
        m_remote_srv_index = srv_index;
        g_remoteSrvPool->index_session_inc(m_remote_srv_index);
    }
    virtual ~CVpnRemote()
    {
        g_remoteSrvPool->index_session_dec(m_remote_srv_index);
    }
public:
    int init()
    {
        return 0;
    }
    void free()
    {
        this->free_handle();
        return;
    }

private:
    int recv_handle(char *buf, int buf_len)
    {
        _LOG_ERROR("CRemote in client not recv");
        return -1;
    }  
    
public:
	int send_client_iobusy_msg(bool isBusy);
    int send_client_close_msg();
    int send_client_connect_msg(char *buf, int buf_len);
    int send_data_msg(char *buf, int buf_len);

public:
    int m_remote_srv_index;
};

#endif

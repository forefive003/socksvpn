#ifndef _RELAY_REMOTE_H
#define _RELAY_REMOTE_H

#include "CNetObjPool.h"

/*本类只是存放一下远端的ip, 没有其他作用
发送报文实际调用的是CCSocksSrv的接口
*/
class CRemote : public CBaseRemote
{
public:
    CRemote(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner, int srv_index) : CBaseRemote(ipaddr, port,fd, owner)
    {
        m_remote_srv_index = srv_index;
        g_socksNetPool->index_session_inc(m_remote_srv_index);
    }
    virtual ~CRemote()
    {
        g_socksNetPool->index_session_dec(m_remote_srv_index);
    }

public:
    int init()
    {
        return 0;
    }
    void free()
    {
        this->free_handle();
    }
    int recv_handle(char *buf, int buf_len)
    {
        _LOG_ERROR("CRemote in relay not recv");
        return -1;
    }
public:
    int send_client_close_msg();
    int send_client_connect_msg(char *buf, int buf_len);
    int send_data_msg(char *buf, int buf_len);

private:
	/*用于查找socket连接*/
    int m_remote_srv_index;
};

#endif

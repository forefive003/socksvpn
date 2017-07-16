
#ifndef _SRV_REMOTE_H
#define _SRV_REMOTE_H


class CRemote : public CBaseRemote
{
public:
    CRemote(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner) : CBaseRemote(ipaddr, port, fd, owner)
    {
        m_request_time = 0;
    }
    virtual ~CRemote()
    {
    }

public:
    int send_data_msg(char *buf, int buf_len);
    int send_client_close_msg()
    {
        /*nothing to send, will close remote in free_handle*/
        return 0;
    }

private:
    int recv_handle(char *buf, int buf_len);
    int connect_handle(BOOL result);

public:
    uint64_t m_request_time;
};

#endif

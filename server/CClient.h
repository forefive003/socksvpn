#ifndef _SRV_CLIENT_H
#define _SRV_CLIENT_H

class CClient: public CBaseClient
{
public:
	CClient(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner) : CBaseClient(ipaddr, port, fd, owner)
    {
        _LOG_DEBUG("construct client");
    }

    virtual ~CClient()
    {
        _LOG_DEBUG("destruct client");
    }

public:
    int init()
    {
        _LOG_DEBUG("no need to init client on server");
        return 0;
    }
    void free()
    {
        /*what do to here is similar to free_handle(), plus delete this.
        because no fd read job*/
        this->free_handle();
        return;
    }

    int recv_handle(char *buf, int buf_len)
    {
        _LOG_ERROR("client on server not recv directly.");
        return -1;
    }
public:
    int send_data(char *buf, int buf_len);

    int send_data_msg(char *buf, int buf_len);
    int send_connect_result(BOOL result);
    int send_remote_close_msg();
};

#endif

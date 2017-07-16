#ifndef _CLI_REMOTE_H
#define _CLI_REMOTE_H

class CRemote : public CBaseRemote
{
public:
    CRemote(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner) : CBaseRemote(ipaddr, port, fd, owner)
    {
    }
    virtual ~CRemote()
    {
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
    int send_client_close_msg();
    int send_client_connect_msg(char *buf, int buf_len);
    int send_data_msg(char *buf, int buf_len);
};

#endif

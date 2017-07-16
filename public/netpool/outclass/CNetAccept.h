
#ifndef _NET_ACPT_H
#define _NET_ACPT_H

class CNetAccept
{
public:
    CNetAccept(uint16_t port);
    virtual ~CNetAccept();

public:
    virtual int init();
    virtual void free();
    
private:
    int register_accept();
    void unregister_accept();

    static void _accept_callback(int  fd, void* param1);
    virtual int accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port) = 0;

    static void _free_callback(int fd, void* param1);
    virtual void free_handle();

public:   
    int m_listen_fd;
    uint16_t m_listen_port;
};


#endif


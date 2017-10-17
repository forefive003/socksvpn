
#ifndef _NET_ACPT_H
#define _NET_ACPT_H

#include "engine_ip.h"

#ifdef _WIN32

#if  defined(DLL_EXPORT_NP)
class _declspec(dllexport) CNetAccept
#elif defined(DLL_IMPORT_NP)
class _declspec(dllimport) CNetAccept
#else
class CNetAccept
#endif

#else
class CNetAccept
#endif

{
public:
    CNetAccept(uint16_t port);
    CNetAccept(const char *local_ipstr, uint16_t port);
    virtual ~CNetAccept();

public:
    virtual int init();
    virtual void free();
    
    int register_accept();
    void unregister_accept();

private:
    static void _accept_callback(int  fd, void* param1);
    virtual int accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port) = 0;

    static void _free_callback(int fd, void* param1);
    virtual void free_handle();

public:   
    int m_listen_fd;
    uint16_t m_listen_port;
    char m_local_ipstr[IP_DESC_LEN + 1];
};


#endif


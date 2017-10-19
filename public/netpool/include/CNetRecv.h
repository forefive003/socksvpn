
#ifndef _NET_RECV_H
#define _NET_RECV_H

#include <stdexcept>
#include "list.h"
#include "squeue.h"
#include "engine_ip.h"
#include "CSocksSendQ.h"

#ifdef _WIN32

#pragma warning (disable:4290)

#if  defined(DLL_EXPORT_NP)
class _declspec(dllexport) CNetRecv
#elif defined(DLL_IMPORT_NP)
class _declspec(dllimport) CNetRecv
#else
class CNetRecv
#endif

#else
class CNetRecv
#endif

{
public:
    CNetRecv(uint32_t ipaddr, uint16_t port, int fd) throw(std::runtime_error);
    CNetRecv(char *ipstr, uint16_t port, int fd) throw(std::runtime_error);
    CNetRecv(uint32_t ipaddr, uint16_t port) throw(std::runtime_error);
    CNetRecv(char *ipstr, uint16_t port) throw(std::runtime_error);
    virtual ~CNetRecv();

public:
    void set_thrd_index(int thrd_index);
    BOOL is_connected();
    void set_async_write_flag(bool is_async);

    virtual int init();
    virtual void free();    
    virtual int send_data(char *buf, int buf_len);

    int register_write();
    int register_connect();
    int register_read();

    int pause_read();
    int resume_read();

private:
    virtual int send_pre_handle();
    virtual int send_post_handle();

    virtual int connect_handle(BOOL result);
    virtual int recv_handle(char *buf, int buf_len) = 0;
    virtual void free_handle();

    static void _send_callback(int  fd, void* param1);
    static void _connect_callback(int  fd, void* param1);
    static void _recv_callback(int  fd, void* param1, char *recvBuf, int recvLen);
    static void _free_callback(int fd, void* param1);
    
private:
    void init_common_data();
    void destroy_common_data();

    int init_peer_ipinfo(uint32_t ipaddr, uint16_t port);
    int init_peer_ipinfo(char *ipstr, uint16_t port);
    int init_local_ipinfo();
    
public:
    int m_fd;

    uint32_t m_ipaddr;
	char m_ipstr[IP_DESC_LEN + 1];
    uint16_t m_port;

    uint32_t m_local_ipaddr;
	char m_local_ipstr[IP_DESC_LEN + 1];
    uint16_t m_local_port;

    CSocksSendQ m_send_q;
    unsigned int m_send_q_busy_cnt;

    int m_thrd_index;

private:    
    bool m_is_async_write;

    MUTEX_TYPE m_free_lock; /*send_data and free maybe called in diffrent thread,
                                so protect them*/
    bool m_is_connected;
    bool m_is_register_write;
    bool m_is_freeing;
    bool m_is_pause_read;
    bool m_is_fwd_server; /*whether forward data that recv by self to another fd,
                        if yes, not pause read when local send busy, otherwith pause,
                        default is true*/
};

#endif

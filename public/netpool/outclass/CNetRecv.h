
#ifndef _NET_RECV_H
#define _NET_RECV_H

#include <stdexcept>
#include "ipparser.h"
#include "list.h"
#include "squeue.h"


#define DEF_WR_TIMEOUT 3
#define DEF_RD_TIMEOUT 3

#define BUF_NODE_SIZE 1500

typedef struct 
{
    struct list_head node;

    /* data */
    uint16_t consume_pos;
    uint16_t produce_pos;
    char data[BUF_NODE_SIZE];
}buf_node_t;

class CNetRecv
{
public:
    CNetRecv(uint32_t ipaddr, uint16_t port, int fd) throw(std::runtime_error);
    CNetRecv(char *ipstr, uint16_t port, int fd) throw(std::runtime_error);
    CNetRecv(uint32_t ipaddr, uint16_t port) throw(std::runtime_error);
    CNetRecv(char *ipstr, uint16_t port) throw(std::runtime_error);
    virtual ~CNetRecv();

public:
    void set_thrd_index(unsigned int thrd_index);
    int notify_write();
    BOOL is_connected();
    void init_async_write_resource(void* (*new_mem_func)(),
            void (*free_mem_func)(void*));
    void free_async_write_resource();

    virtual int init();
    virtual void free();    
    virtual int send_data(char *buf, int buf_len);

    int register_write();
    void unregister_write();

    int register_connect();
    void unregister_connect();

    int register_read();
    void unregister_read();

private:
    virtual int send_handle();
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
    int send_buf_node(buf_node_t *buf_node);
    
public:
    int m_fd;

    uint32_t m_ipaddr;
    char m_ipstr[HOST_IP_LEN + 1];
    uint16_t m_port;

    uint32_t m_local_ipaddr;
    char m_local_ipstr[HOST_IP_LEN + 1];
    uint16_t m_local_port;

private:
    bool m_is_connected;
    unsigned int m_thrd_index;
    bool m_is_register_write;
    bool m_is_register_read;
    bool m_is_register_connect;

    bool m_is_async_write;
    uint32_t m_mem_node_size;
    
    void* (*m_new_mem_func)();
    void (*m_free_mem_func)(void* mem_node);
    sq_head_t m_data_queue;
    pthread_spinlock_t m_queue_lock;
    buf_node_t* m_cur_send_node;
    bool m_is_write_failed;

    MUTEX_TYPE m_register_lock; /*send_data and free maybe called in diffrent thread,
                                so protect them*/
    bool m_is_freed;
};

#endif

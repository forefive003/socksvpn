
#ifndef _NET_CONNECTION_H
#define _NET_CONNECTION_H

class CBaseConnection
{
public:
    CBaseConnection();
    virtual ~CBaseConnection();
    
public:
    uint32_t get_client_inner_ipaddr();
    uint16_t get_client_inner_port();

    uint32_t get_client_ipaddr();
    uint16_t get_client_port();
    int get_client_fd();

    uint32_t get_remote_ipaddr();
    uint16_t get_remote_port();
    int get_remote_fd();

    int fwd_client_data_msg(char *buf, int buf_len);
    int fwd_remote_data_msg(char *buf, int buf_len);

    void notify_client_close();
    void notify_remote_close();
    void free_client();
    void free_remote();
public:
    int attach_client(CBaseClient *client);
    int attach_remote(CBaseRemote *remote);
    void detach_client();
    void detach_remote();

    void inc_ref();
    void dec_ref();
    
    virtual void print_statistic(FILE *pFd) = 0;
    void destroy();
public:
    MUTEX_TYPE m_remote_lock;
    CBaseClient *m_client;
    CBaseRemote *m_remote;

    uint64_t m_send_client_data_cnt;
    uint64_t m_send_remote_data_cnt;
private:
    MUTEX_TYPE m_ref_lock;
    uint32_t m_refcnt;
};


extern uint64_t g_total_data_req_cnt;
extern uint64_t g_total_data_resp_cnt;

#endif

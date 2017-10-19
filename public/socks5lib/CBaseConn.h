
#ifndef _NET_CONNECTION_H
#define _NET_CONNECTION_H

#define  FRAME_LEN  900

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

    void set_client_busy(bool is_busy)
    {
        m_is_client_busy = is_busy;
    }
    bool is_client_busy()
    {
        return m_is_client_busy;
    }

    void set_remote_busy(bool is_busy)
    {
        m_is_remote_busy = is_busy;
    }
    bool is_remote_busy()
    {
        return m_is_remote_busy;
    }

    void set_client_pause_read(bool is_pause)
    {
        m_is_client_pause_read = is_pause;
    }
    bool is_client_pause_read()
    {
        return m_is_client_pause_read;
    }

    void set_remote_pause_read(bool is_pause)
    {
        m_is_remote_pause_read = is_pause;
    }
    bool is_remote_pause_read()
    {
        return m_is_remote_pause_read;
    }
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
    MUTEX_TYPE m_event_lock;

    MUTEX_TYPE m_remote_lock;
    CBaseClient *m_client;
    CBaseRemote *m_remote;

    uint64_t m_setup_time;
    
    uint64_t m_send_client_data_cnt;
    uint64_t m_send_remote_data_cnt;

    uint64_t m_send_client_bytes;
    uint64_t m_send_remote_bytes;

    bool m_is_client_busy;
    bool m_is_remote_busy;
    bool m_is_client_pause_read;
    bool m_is_remote_pause_read;

private:
    MUTEX_TYPE m_ref_lock;
    uint32_t m_refcnt;
};


extern uint64_t g_total_data_req_cnt;
extern uint64_t g_total_data_req_byte;
extern uint64_t g_total_data_resp_cnt;
extern uint64_t g_total_data_resp_byte;

#endif

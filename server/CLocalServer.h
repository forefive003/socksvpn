#ifndef _SRV_LOCALSERVER_H
#define _SRV_LOCALSERVER_H


class CLocalServer : public CNetRecv
{
public:
    CLocalServer(uint32_t ipaddr, uint16_t port) : CNetRecv(ipaddr, port)
    {
        //this->set_thrd_index(0);

        memset(m_recv_buf, 0, sizeof(m_recv_buf));
        m_recv_len = 0;

        m_latest_alive_time = 0;
        m_self_pool_index = -1;
        _LOG_INFO("create local server(peer %s:%u)", m_ipstr, m_port);
    }
    virtual ~CLocalServer()
    {
        _LOG_INFO("destroy local server(peer %s:%u)", m_ipstr, m_port);
    }

private:
    int pdu_handle(char *pdu_buf, int pdu_len);

    int send_pre_handle();
    int send_post_handle();

    int connect_handle(BOOL result);
    int recv_handle(char *buf, int buf_len);
    void free_handle();

public:
    int send_register();
    int send_keepalive();
    void set_self_pool_index(int index);

    void print_statistic(FILE* pFd);
    
private:
    int msg_request_handle(PKT_R2S_HDR_T *r2shdr, char *data_buf, int data_len);
    int msg_client_close_handle(PKT_R2S_HDR_T *r2shdr, char *data_buf, int data_len);
    int msg_data_handle(PKT_R2S_HDR_T *r2shdr, char *data_buf, int data_len);

private:
    char m_recv_buf[2048];
    int m_recv_len;

public:
    uint64_t m_latest_alive_time;
    int m_self_pool_index;
};

#endif

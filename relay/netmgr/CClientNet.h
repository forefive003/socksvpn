
#ifndef _RELAY_CLINETNET_H
#define _RELAY_CLINETNET_H

class CClientNet : public CNetRecv
{
public:
    CClientNet(uint32_t ipaddr, uint16_t port, int fd) : CNetRecv(ipaddr, port, fd)
    {
        this->set_thrd_index(0);
        
        memset(m_recv_buf, 0, sizeof(m_recv_buf));
        m_recv_len = 0;

        m_inner_ipaddr = 0;
        m_inner_port = 0;

        m_is_authed = FALSE;
        
        memset(m_username, 0, sizeof(m_username));
        memset(m_passwd, 0, sizeof(m_passwd));


        m_send_remote_close_cnt = 0;
        m_send_connect_result_cnt = 0;
        m_send_data_cnt = 0;
        m_recv_client_close_cnt = 0;
        m_recv_connect_request_cnt = 0;
        m_recv_data_cnt = 0;

        _LOG_INFO("construct clientserver, %s:%u, fd %d", m_ipstr, m_port, m_fd);
    }
    virtual ~CClientNet()
    {
        _LOG_INFO("destruct clientserver, %s:%u, fd %d", m_ipstr, m_port, m_fd);
    }

    int send_auth_result_msg(BOOL auth_ok);
    BOOL is_self(uint32_t pub_ipaddr, uint16_t pub_port);
    void set_user_passwd(char *username, char *passwd);
    void set_inner_info(uint32_t inner_ipaddr, uint16_t inner_port);
    void print_statistic(FILE *pFd);
private:
    int pdu_handle(char *pdu_buf, int pdu_len);
    int recv_handle(char *buf, int buf_len);
    void free_handle();
    
private:
    int parse_connect_req_msg(char *data_buf, int data_len);

    int msg_auth_handle(PKT_C2R_HDR_T *c2rhdr, char *data_buf, int data_len);
    int msg_data_handle(PKT_C2R_HDR_T *c2rhdr, char *data_buf, int data_len);
    int msg_connect_handle(PKT_C2R_HDR_T *c2rhdr, char *data_buf, int data_len);
    int msg_client_closed_handle(PKT_C2R_HDR_T *c2rhdr, char *data_buf, int data_len);

private:
    char m_recv_buf[2048];
    int m_recv_len;

    BOOL m_is_authed;
    char m_username[MAX_USERNAME_LEN + 1];
    char m_passwd[MAX_PASSWD_LEN + 1];
    uint32_t m_inner_ipaddr;
    uint16_t m_inner_port;
    char m_inner_ipstr[HOST_IP_LEN + 1];
    
public:
    uint64_t m_send_remote_close_cnt;
    uint64_t m_send_connect_result_cnt;
    uint64_t m_send_data_cnt;
    uint64_t m_recv_client_close_cnt;
    uint64_t m_recv_connect_request_cnt;
    uint64_t m_recv_data_cnt;
};

#endif
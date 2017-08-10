
#ifndef _RELAY_SOCKSSERVER_H
#define _RELAY_SOCKSSERVER_H

#include "relay_pkt_def.h"
#include "CNetRecv.h"
#include "CServerCfg.h"


class CSocksSrv : public CNetRecv
{
public:
    CSocksSrv(uint32_t ipaddr, uint16_t port, int fd) : CNetRecv(ipaddr, port, fd)
    {
        this->set_thrd_index(1);
        
        memset(m_recv_buf, 0, sizeof(m_recv_buf));
        m_recv_len = 0;

        m_update_time = 0;
        m_inner_ipaddr = 0;

        m_client_cnt = 0;
        m_send_client_close_cnt = 0;
        m_send_client_connect_cnt = 0;
        m_send_data_cnt = 0;
        m_recv_remote_close_cnt = 0;
        m_recv_connect_result_cnt = 0;
        m_recv_data_cnt = 0;
        m_recv_srv_alive_cnt = 0;
        _LOG_INFO("construct sockserver, %s:%u, fd %d", m_ipstr, m_port, m_fd);
    }
    virtual ~CSocksSrv()
    {
        _LOG_INFO("destruct sockserver, %s:%u, fd %d", m_ipstr, m_port, m_fd);
    }

public:
    void set_config(CServerCfg *srvCfg);

    void add_client();
    void del_client();

    BOOL is_self(uint32_t pub_ipaddr, uint32_t inner_ipaddr);
    BOOL is_self(uint32_t pub_ipaddr, char *username);
    BOOL is_self(const char* pub_ipstr, const char* inner_ipstr);
    
    void print_statistic(FILE *pFd);
    void set_inner_info(uint32_t inner_ipaddr, uint16_t inner_port);
    
private:
    int pdu_handle(char *pdu_buf, int pdu_len);
    int recv_handle(char *buf, int buf_len);
    void free_handle();
    
private:
    int msg_srv_reg_handle(char *data_buf, int data_len);
    int msg_data_handle(PKT_S2R_HDR_T *s2rhdr, char *data_buf, int data_len);
    int msg_connect_result_handle(PKT_S2R_HDR_T *s2rhdr, char *data_buf, int data_len);
    int msg_remote_closed_handle(PKT_S2R_HDR_T *s2rhdr, char *data_buf, int data_len);

private:
    uint32_t m_inner_ipaddr;
    uint16_t m_inner_port;
    char m_inner_ipstr[HOST_IP_LEN + 1];

private:
    char m_recv_buf[2048];
    int m_recv_len;
private:
	uint64_t m_update_time;

    CServerCfg m_srvCfg;

public:
    uint32_t m_client_cnt;
    uint64_t m_send_client_close_cnt;
    uint64_t m_send_client_connect_cnt;
    uint64_t m_send_data_cnt;
    uint64_t m_recv_connect_result_cnt;
    uint64_t m_recv_remote_close_cnt;
    uint64_t m_recv_data_cnt;
    uint64_t m_recv_srv_alive_cnt;
};

#endif

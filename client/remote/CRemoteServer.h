#ifndef _CLI_REMOTESERVER_H
#define _CLI_REMOTESERVER_H

class CRemoteServer : public CNetRecv
{
public:
    CRemoteServer(uint32_t ipaddr, uint16_t port) : CNetRecv(ipaddr, port)
    {
        _LOG_INFO("create remote server(remote %s:%u)", m_ipstr, m_port);
        memset(m_recv_buf, 0, sizeof(m_recv_buf));
        m_recv_len = 0;

        m_is_authed = FALSE;
    }
    virtual ~CRemoteServer()
    {
        _LOG_INFO("destroy remote server(remote %s:%u)", m_ipstr, m_port);
    }

public:
    int send_auth_quest_msg();
    BOOL is_authed();
    void reset_authed();
    
private:
    int pdu_handle(char *pdu_buf, int pdu_len);

    int connect_handle(BOOL result);
    int recv_handle(char *buf, int buf_len);
    void free_handle();

    int auth_result_msg_handle(BOOL result);
private:
    int build_authen_msg(char *buf);
    BOOL parse_authen_result_msg(char *buf, int buf_len);
    BOOL parse_connect_result_msg(char *buf, int buf_len, int *remoteIpaddr);

private:
    char m_recv_buf[2048];
    int m_recv_len;

    BOOL m_is_authed;
};

extern MUTEX_TYPE m_remote_srv_lock;
extern CRemoteServer *g_RemoteServ;
extern BOOL is_remote_authed();
extern BOOL is_remote_connected();
#endif

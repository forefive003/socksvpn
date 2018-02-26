#ifndef _SRV_CLIENT_H
#define _SRV_CLIENT_H

class CClient: public CBaseClient
{
public:
	CClient(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner) : CBaseClient(ipaddr, port, fd, owner)
    {       
        _LOG_DEBUG("construct client"); 

        m_sock_ver = SOCKS_INVALID;
        memset(m_recv_buf, 0, sizeof(m_recv_buf));
        m_recv_len = 0;

        m_status = CLI_INIT;     
    }
    virtual ~CClient()
    {
        _LOG_DEBUG("destruct client");
    }

public:
    int recv_handle(char *buf, int buf_len);

    int get_client_status();
    void set_client_status(CLI_STATUS_E status);

    int send_connect_result(BOOL result);

private:
    int try_connect_remote();

    int socks5_connect_request_handle(char *buf, int buf_len);
    int socks5_auth_request_handle(char *buf, int buf_len);
    int socks5_handshake_request_handle(char *buf, int buf_len);
    int socks4_handshake_request_handle(char *buf, int buf_len);

private:
    CLI_STATUS_E m_status;
    SOCK_VER_E m_sock_ver;

    char m_recv_buf[512];
    int m_recv_len;

    uint32_t m_remote_ipaddr;
    char m_remote_domain[HOST_DOMAIN_LEN + 1];
    uint16_t m_remote_dstport;
};

#endif

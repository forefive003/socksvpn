#ifndef _SRV_REMOTE_H
#define _SRV_REMOTE_H


class CSrvRemote : public CBaseRemote
{
public:
    CSrvRemote(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner, int version) : CBaseRemote(ipaddr, port, fd, owner)
    {
        m_status = SOCKS_INIT;
        m_version = version;
    }
    virtual ~CSrvRemote()
    {
    }

private:
    int connect_handle(BOOL result);
    int recv_handle(char *buf, int buf_len);

    int socks4_handshake_resp_handle(char *buf, int buf_len);
    int socks5_handshake_resp_handle(char *buf, int buf_len);
    int socks5_auth_resp_handle(char *buf, int buf_len);
    int socks5_connect_resp_handle(char *buf, int buf_len);

public:
    void set_remote_status(SOCKS_STATUS_E status);
    int get_remote_status();

    void set_real_server(uint32_t real_serv, uint16_t real_port)
    {
        m_real_remote_ipaddr = real_serv;
        m_real_remote_port = real_port;
    }

    void set_real_domain(char *domain_name)
    {
        memset(m_remote_domain, 0, HOST_DOMAIN_LEN + 1);
        strncpy(m_remote_domain, domain_name, HOST_DOMAIN_LEN);
    }
private:
    uint32_t m_real_remote_ipaddr; /*真实的远端服务器ip*/
    uint16_t m_real_remote_port; /*真实的远端服务器端口*/
    char m_remote_domain[HOST_DOMAIN_LEN + 1];

public:
    SOCKS_STATUS_E m_status;
    int m_version; /*4 or 5*/
};

#endif

#ifndef _SRV_REMOTE_H
#define _SRV_REMOTE_H


class CSrvRemote : public CBaseRemote
{
public:
    CSrvRemote(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner) : CBaseRemote(ipaddr, port, fd, owner)
    {
        m_status = SOCKS_INIT;
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

public:
    SOCKS_STATUS_E m_status;
};

#endif

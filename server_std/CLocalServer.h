#ifndef _SRV_LOCALSERVER_H
#define _SRV_LOCALSERVER_H


class CLocalServer : public CNetAccept
{
public:   
	CLocalServer(uint16_t port) : CNetAccept(port)
	{
		_LOG_INFO("construct LocalServer on port %u", m_listen_port);
	}
    virtual ~CLocalServer()
    {
        _LOG_INFO("destruct LocalServer on port %u", m_listen_port);
    }

public:
    static CLocalServer* instance(uint16_t port);

private:
    int accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port);
};

extern CLocalServer *g_LocalServ;

#endif

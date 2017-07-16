#ifndef _RELAY_CLISERVER_H
#define _RELAY_CLISERVER_H

class CClientServer : public CNetAccept
{
public:   
	CClientServer(uint16_t port) : CNetAccept(port)
	{
		_LOG_INFO("construct client server on port %u", m_listen_port);
	}
    virtual ~CClientServer()
    {
        _LOG_INFO("destruct client server on port %u", m_listen_port);
    }

public:
    static CClientServer* instance(uint16_t port);

private:
    int accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port);
};


extern CClientServer *g_ClientServ;

#endif

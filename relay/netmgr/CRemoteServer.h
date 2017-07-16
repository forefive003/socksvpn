#ifndef _RELAY_REMOTESERVER_H
#define _RELAY_REMOTESERVER_H


class CRemoteServer : public CNetAccept
{
public:   
    CRemoteServer(uint16_t port) : CNetAccept(port)
    {
        _LOG_INFO("construct remote server on port %u", m_listen_port);
    }
    virtual ~CRemoteServer()
    {
        _LOG_INFO("destruct remote server on port %u", m_listen_port);
    }

public:
    static CRemoteServer* instance(uint16_t port)
    {
        static CRemoteServer *remoteSrv = NULL;

        if(remoteSrv == NULL)
        {
            remoteSrv = new CRemoteServer(port);
        }
        return remoteSrv;
    }

private:
    int accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port);
};

extern CRemoteServer *g_RemoteServ;

#endif

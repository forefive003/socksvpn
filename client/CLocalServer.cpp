#include "commtype.h"
#include "logproc.h"
#include "CNetAccept.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CClient.h"
#include "CLocalServer.h"
#include "socks_client.h"
#include "CSocksMem.h"

CLocalServer *g_LocalServ = NULL;

CLocalServer* CLocalServer::instance(uint16_t port)
{
    static CLocalServer *localSrv = NULL;

    if(localSrv == NULL)
    {
        localSrv = new CLocalServer(port);
    }
    return localSrv;
}

int CLocalServer::accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port)
{
    CConnection *pConn = new CConnection();
    g_ConnMgr->add_conn(pConn);

    CClient *pClient = new CClient(client_ip, client_port, conn_fd, pConn);
    //pClient->set_inner_info(client_ip,client_port);
    pClient->init_async_write_resource(socks_malloc, socks_free);
    g_total_client_cnt++;
    pConn->attach_client(pClient);

    if (0 != pClient->init())
    {
        pConn->detach_client();
        g_ConnMgr->del_conn(pConn);
        delete pClient;
        delete pConn;
        return -1;
    }

    return 0;
}

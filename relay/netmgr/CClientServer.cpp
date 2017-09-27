#include <netinet/tcp.h>
#include "commtype.h"
#include "logproc.h"
#include "CNetAccept.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "CNetRecv.h"
#include "CClientNet.h"
#include "CClientNetMgr.h"
#include "socks_relay.h"
#include "CClientServer.h"
#include "CSocksMem.h"

CClientServer *g_ClientServ = NULL;

CClientServer* CClientServer::instance(uint16_t port)
{
    static CClientServer *clientSrv = NULL;

    if(clientSrv == NULL)
    {
        clientSrv = new CClientServer(port);
    }
    return clientSrv;
}

int CClientServer::accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port)
{
    int keepalive = 1; // 开启keepalive属性
    int keepidle = 10; // 如该连接在60秒内没有任何数据往来,则进行探测
    int keepinterval = 5; // 探测时发包的时间间隔为5 秒
    int keepcount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    setsockopt(conn_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
    setsockopt(conn_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
    setsockopt(conn_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
    setsockopt(conn_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));

    CClientNet *clientNet = new CClientNet(client_ip, client_port, conn_fd);
//    clientNet->init_async_write_resource(socks_malloc, socks_free);
    if (0 != clientNet->init())
    {
        delete clientNet;
        return -1;
    }

    /*添加到管理中*/
    g_ClientNetMgr->add_client_server(clientNet);
    return 0;
}

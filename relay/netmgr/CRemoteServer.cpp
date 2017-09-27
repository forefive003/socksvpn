#include <netinet/tcp.h>
#include "commtype.h"
#include "logproc.h"
#include "CNetAccept.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CRemoteServer.h"
#include "CSocksSrv.h"
#include "CSocksSrvMgr.h"
#include "CSocksMem.h"

CRemoteServer *g_RemoteServ = NULL;

int CRemoteServer::accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port)
{
	int keepalive = 1; // 开启keepalive属性
	int keepidle = 10; // 如该连接在60秒内没有任何数据往来,则进行探测
	int keepinterval = 5; // 探测时发包的时间间隔为5 秒
	int keepcount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	setsockopt(conn_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
	setsockopt(conn_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
	setsockopt(conn_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
	setsockopt(conn_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));

	CSocksSrv *sockSrv = new CSocksSrv(client_ip, client_port, conn_fd);
//	sockSrv->init_async_write_resource(socks_malloc, socks_free);
    if (0 != sockSrv->init())
    {
        delete sockSrv;
        return -1;
    }
    
    g_SocksSrvMgr->add_socks_server(sockSrv);
    return 0;
}

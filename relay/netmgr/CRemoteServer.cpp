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
	int index = g_socksNetPool->add_conn_obj((CNetRecv*)sockSrv);
    if (-1 == index)
    {
        _LOG_ERROR("fail to add new conn obj for socks server");
        delete sockSrv;
        return -1;
    }
    sockSrv->set_self_pool_index(index);

    if (0 != sockSrv->init())
    {
    	g_socksNetPool->del_conn_obj(index);
        delete sockSrv;
        return -1;
    }
    
    return 0;
}

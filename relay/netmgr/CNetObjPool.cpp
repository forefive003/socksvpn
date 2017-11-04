
#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "CNetRecv.h"
#include "CClient.h"
#include "CRemote.h"
#include "CClientNet.h"

#include "CNetObjPool.h"
#include "CNetObjSet.h"
#include "CNetObjMgr.h"

#include "socks_relay.h"

CNetObjPool *g_clientNetPool = NULL;
CNetObjPool *g_socksNetPool = NULL;
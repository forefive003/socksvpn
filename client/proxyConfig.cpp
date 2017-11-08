#include "commtype.h"
#include "utilcommon.h"
#include "logproc.h"
#include "engine_ip.h"
#include "common_def.h"
#include "proxyConfig.h"

#include "CNetAccept.h"
#include "CNetRecv.h"
#include "CRemoteServer.h"
#include "CLocalServer.h"
#include "CRemoteServerPool.h"

#ifdef _WIN32
#include "procMgr.h"
#else
#define MAX_PATH 256
#endif

#ifdef _WIN32
static char g_proxy_cfg_dir[MAX_PATH + 1] = {0};
#endif

static proxy_cfg_t g_proxy_cfg;

/*禁止注入的进程*/
int g_proxy_forbit_inject_procs_cnt = 0;
char g_proxy_forbit_inject_procs[PROXY_CFG_MAX_PROC][64];

/*能够注入的进程*/
int g_proxy_procs_cnt = 0;
char g_proxy_procs[PROXY_CFG_MAX_PROC][64];

/*能够选择的代理服务器*/
int g_servers_cnt = 0;
int g_servers_ipaddr[PROXY_MAX_SERVER] = {0};
char g_servers[PROXY_MAX_SERVER][IP_DESC_LEN];

/*当前状态*/
bool g_is_start = false;

int g_log_level = -1;
int g_thrd_cnt = 0;

const char* g_socks_status_desc[] =
{
    "INVALID",
    "INIT",
    "HANDSHAKING",
    "AUTHING",
    "CONNECTING",
    "CONNECTED",
    "CONNECTED_FAILED",
};

const char* g_proxy_cfg_proto_desc[] = {
	"Invalid",
	"Socks4",
	"Socks5"
};
const char* g_proxy_cfg_type_desc[] = {
	"Invalid Mode",
	"VPN Server Mode",
	"Proxy Server Mode"
};
const char* g_proxy_cfg_encry_type_desc[] = {
	"Invalid",
	"None",
	"Xor",
	"Rc4"
};

int proxy_cfg_get_type(const char* desc)
{
	if (strncmp(desc, g_proxy_cfg_type_desc[PROXY_VPN], strlen(g_proxy_cfg_type_desc[PROXY_VPN])) == 0)
	{
		return PROXY_VPN;
	}
	else if (strncmp(desc, g_proxy_cfg_type_desc[PROXY_SERVER], strlen(g_proxy_cfg_type_desc[PROXY_SERVER])) == 0)
	{
		return PROXY_SERVER;
	}

	return -1;
}

int proxy_cfg_get_proto(const char* desc)
{
	if (strncmp(desc, g_proxy_cfg_proto_desc[SOCKS_4], strlen(g_proxy_cfg_proto_desc[SOCKS_4])) == 0)
	{
		return SOCKS_4;
	}
	else if (strncmp(desc, g_proxy_cfg_proto_desc[SOCKS_5], strlen(g_proxy_cfg_proto_desc[SOCKS_5])) == 0)
	{
		return SOCKS_5;
	}

	return -1;
}

int proxy_cfg_get_encry_type(const char* desc)
{
	if (strncmp(desc, g_proxy_cfg_encry_type_desc[ENCRY_NONE], strlen(g_proxy_cfg_encry_type_desc[ENCRY_NONE])) == 0)
	{
		return ENCRY_NONE;
	}
	else if (strncmp(desc, g_proxy_cfg_encry_type_desc[ENCRY_XOR], strlen(g_proxy_cfg_encry_type_desc[ENCRY_XOR])) == 0)
	{
		return ENCRY_XOR;
	}
	else if (strncmp(desc, g_proxy_cfg_encry_type_desc[ENCRY_RC4], strlen(g_proxy_cfg_encry_type_desc[ENCRY_RC4])) == 0)
	{
		return ENCRY_RC4;
	}

	return -1;
}

void proxy_set_servers(int *srv_ip_arr, int srv_cnt)
{
	if (g_servers_cnt != srv_cnt)
	{
		_LOG_INFO("update servers, cnt %d", srv_cnt);
	}

	BOOL isCfgServerOk = false;

	g_servers_cnt = 0;
	memset(g_servers_ipaddr, 0, sizeof(g_servers_ipaddr));
	memset(g_servers, 0, sizeof(g_servers));

	for(int ii = 0; ii < srv_cnt; ii++)
	{
		g_servers_ipaddr[ii] = srv_ip_arr[ii];
		engine_ipv4_to_str(htonl(g_servers_ipaddr[ii]), g_servers[ii]);

		if (g_proxy_cfg.server_ip == (uint32_t)g_servers_ipaddr[ii])
		{
			isCfgServerOk = true;
		}
	}

	g_servers_cnt = srv_cnt;

	if (!isCfgServerOk)
	{
		g_remoteSrvPool->let_re_auth();
	}

#ifdef _WIN32
	/*通知主窗口更新*/
    ::PostMessage(g_parentWnd, WM_UPDATE_SERVERS_COMB, NULL, NULL);
#endif
}

bool proxy_cfg_forbit_inject_proc_is_exist(char *exe_name)
{
	for (int ii = 0; ii < g_proxy_forbit_inject_procs_cnt; ii++)
	{
		if (strncmp(exe_name, g_proxy_forbit_inject_procs[ii], 64) == 0)
		{
			return true;
		}
	}

	return false;
}

bool proxy_cfg_proc_is_exist(char *exe_name)
{
	for (int ii = 0; ii < g_proxy_procs_cnt; ii++)
	{
		if (strncmp(exe_name, g_proxy_procs[ii], 64) == 0)
		{
			return true;
		}
	}

	return false;
}

int proxy_cfg_add_proc(char *exe_name)
{
	if (g_proxy_procs_cnt >= PROXY_CFG_MAX_PROC)
	{
		return -1;
	}

	strncpy(g_proxy_procs[g_proxy_procs_cnt], exe_name, 64);
	g_proxy_procs[g_proxy_procs_cnt][63] = 0;
	g_proxy_procs_cnt++;
	
	proxy_cfg_save();

	return 0;
}

void proxy_cfg_del_proc(char *exe_name)
{
	bool isfound = false;

	for (int ii = 0; ii < g_proxy_procs_cnt; ii++)
	{
		if (isfound == false)
		{
			if (strncmp(exe_name, g_proxy_procs[ii], 64) == 0)
			{
				isfound = true;
			}
			else
			{
				continue;
			}
		}

		/*move elment*/
		if (ii == (PROXY_CFG_MAX_PROC-1))
		{
			memset(g_proxy_procs[ii], 0, 64);
		}
		else
		{
			strncpy(g_proxy_procs[ii], g_proxy_procs[ii+1], 64);
			g_proxy_procs[ii][63] = 0;
		}
	}

	if (isfound)
	{
		g_proxy_procs_cnt--;
	}

	proxy_cfg_save();
}


static inline bool _proxy_is_cfg_need_restart(proxy_cfg_t *config)
{
	if (config->proxy_type != g_proxy_cfg.proxy_type
		|| config->vpn_ip != g_proxy_cfg.vpn_ip
		|| config->vpn_port != g_proxy_cfg.vpn_port)
	{
		return true;
	}

	if (config->server_ip != g_proxy_cfg.server_ip
			|| config->is_auth != g_proxy_cfg.is_auth
			|| config->proxy_proto != g_proxy_cfg.proxy_proto
			|| (strncmp(config->username, g_proxy_cfg.username, sizeof(config->username)) != 0)
			|| (strncmp(config->passwd, g_proxy_cfg.passwd, sizeof(config->passwd)) != 0))
	{
		return true;
	}

	return false;
}

int proxy_cfg_set(proxy_cfg_t *config)
{
#ifdef _WIN32
	/*windows上 注入配置端口不变*/
	config->config_port = g_proxy_cfg.config_port;
	config->proxy_proto = g_proxy_cfg.proxy_proto;

	if (g_is_start)
	{
		if(config->local_port != g_proxy_cfg.local_port)
		{
			/*停止local*/
			if (g_LocalServ != NULL)
			{
				if (MessageBox(NULL, "本地监听端口变化, 需要重启监听服务, 确定要修改吗?", "提示", MB_OKCANCEL) != IDOK)
				{
					return 0;
				}

				g_LocalServ->free();

				g_LocalServ = new CLocalServer(config->local_port);
			    if(0 != g_LocalServ->init())
			    {
			    	MessageBox(NULL, _T("local server init failed!"), _T("Error"), MB_OK);
			        return -1;
			    }				
			}
		}

		if (_proxy_is_cfg_need_restart(config))
		{
			if (MessageBox(NULL, "代理配置变化, 当前已启动代理将重新生效, 确定要修改吗？", "提示", MB_OKCANCEL) != IDOK)
			{
				return 0;
			}

			/*需要先停止所有代理, 再启动所有代理*/
			proxy_proc_mgr_stop();

			/*remote server*/
			g_remoteSrvPool->free();
			
			memcpy(&g_proxy_cfg, config, sizeof(proxy_cfg_t));
			proxy_proc_mgr_start();
		}
	}
#endif

	memcpy(&g_proxy_cfg, config, sizeof(proxy_cfg_t));
	return proxy_cfg_save();
}

proxy_cfg_t* proxy_cfg_get()
{
    return &g_proxy_cfg;
}

int proxy_cfg_save()
{	
#ifdef _WIN32
	char strTemp[32] = {0};
	char ipstr[IP_DESC_LEN] = {0};

	/*Inject config port*/
	SNPRINTF(strTemp, sizeof(strTemp) - 1, "%d", g_proxy_cfg.config_port);
	WritePrivateProfileString(_T("Inject"), _T("ConfigPort"), strTemp, _T(g_proxy_cfg_dir));

	/*proxy type*/
	WritePrivateProfileString(_T("Proxy_Basic"), _T("ProxyType"), 
		g_proxy_cfg_type_desc[g_proxy_cfg.proxy_type], _T(g_proxy_cfg_dir));
	/*proxy proto*/
	WritePrivateProfileString(_T("Proxy_Basic"), _T("Proto"), 
		g_proxy_cfg_proto_desc[g_proxy_cfg.proxy_proto], _T(g_proxy_cfg_dir));
	
	/*proxy vpn ip and port*/
	engine_ipv4_to_str(htonl(g_proxy_cfg.vpn_ip), ipstr);
	WritePrivateProfileString(_T("Proxy_Basic"), _T("VpnIp"), ipstr, _T(g_proxy_cfg_dir));
	SNPRINTF(strTemp, sizeof(strTemp) - 1, "%d", g_proxy_cfg.vpn_port);
	WritePrivateProfileString(_T("Proxy_Basic"), _T("VpnPort"), strTemp, _T(g_proxy_cfg_dir));
	
	/*sock server ip and port*/
	engine_ipv4_to_str(htonl(g_proxy_cfg.server_ip), ipstr);
	WritePrivateProfileString(_T("Proxy_Basic"), _T("SockServerIp"), ipstr, _T(g_proxy_cfg_dir));

	/*local port*/
	SNPRINTF(strTemp, sizeof(strTemp) - 1, "%d", g_proxy_cfg.local_port);
	WritePrivateProfileString(_T("Proxy_Basic"), _T("LocalPort"), strTemp, _T(g_proxy_cfg_dir));

	/*auth*/
	WritePrivateProfileString(_T("Proxy_Auth"), _T("Username"), g_proxy_cfg.username, _T(g_proxy_cfg_dir));
	WritePrivateProfileString(_T("Proxy_Auth"), _T("Passwd"), g_proxy_cfg.passwd, _T(g_proxy_cfg_dir));
	SNPRINTF(strTemp, sizeof(strTemp) - 1, "%d", g_proxy_cfg.is_auth);
	WritePrivateProfileString(_T("Proxy_Auth"), _T("IsAuth"), strTemp, _T(g_proxy_cfg_dir));

	/*encry*/
	WritePrivateProfileString(_T("Proxy_Encry"), _T("EncryType"), 
		g_proxy_cfg_encry_type_desc[g_proxy_cfg.encry_type], _T(g_proxy_cfg_dir));
	WritePrivateProfileString(_T("Proxy_Encry"), _T("EncryKey"), g_proxy_cfg.encry_key, _T(g_proxy_cfg_dir));

	/*process*/
	SNPRINTF(strTemp, sizeof(strTemp) - 1, "%d", g_proxy_procs_cnt);
	WritePrivateProfileString(_T("Process"), _T("Count"), strTemp, _T(g_proxy_cfg_dir));

	for (int ii = 0; ii < g_proxy_procs_cnt; ii++)
	{
		char nodename[32] = {0};
		SNPRINTF(nodename, 32, "Name_%d", ii+1);

		WritePrivateProfileString(_T("Process"), _T(nodename), _T(g_proxy_procs[ii]), _T(g_proxy_cfg_dir));
	}

	/*forbit process*/
	SNPRINTF(strTemp, sizeof(strTemp) - 1, "%d", g_proxy_forbit_inject_procs_cnt);
	WritePrivateProfileString(_T("Forbit_Process"), _T("Count"), strTemp, _T(g_proxy_cfg_dir));

	for (int ii = 0; ii < g_proxy_forbit_inject_procs_cnt; ii++)
	{
		char nodename[32] = {0};
		SNPRINTF(nodename, 32, "Name_%d", ii+1);

		WritePrivateProfileString(_T("Forbit_Process"), _T(nodename), _T(g_proxy_forbit_inject_procs[ii]), _T(g_proxy_cfg_dir));
	}

	/*status*/
	SNPRINTF(strTemp, sizeof(strTemp) - 1, "%d", g_is_start);
	WritePrivateProfileString(_T("Status"), _T("IsProxyStart"), strTemp, _T(g_proxy_cfg_dir));

	/*other*/
	SNPRINTF(strTemp, sizeof(strTemp) - 1, "%d", g_thrd_cnt);
	WritePrivateProfileString(_T("Other"), _T("ThreadCnt"), strTemp, _T(g_proxy_cfg_dir));

	char logLevelStr[16] = {0};
	if (g_log_level == L_DEBUG)
	{
		SNPRINTF(logLevelStr, 16, "DEBUG");
	}
	else if (g_log_level == L_INFO)
	{
		SNPRINTF(logLevelStr, 16, "INFO");
	}
	else if (g_log_level == L_WARN)
	{
		SNPRINTF(logLevelStr, 16, "WARNING");
	}
	else 
	{
		SNPRINTF(logLevelStr, 16, "ERROR");
	}
	WritePrivateProfileString(_T("Other"), _T("LogLevel"), _T(logLevelStr), _T(g_proxy_cfg_dir));
#endif
	return 0;
}

bool proxy_cfg_is_vpn_type()
{
	return g_proxy_cfg.proxy_type == PROXY_VPN;
}

int proxy_cfg_init()
{
    memset(&g_proxy_cfg, 0, sizeof(proxy_cfg_t));

    g_proxy_procs_cnt = 0;
	memset(g_proxy_procs, 0, sizeof(g_proxy_procs));

#ifdef _WIN32
	SNPRINTF(g_proxy_cfg_dir, MAX_PATH, "%sGoProxy.ini", g_localModDir);
	
	CString strTemp;
	char ipstr[IP_DESC_LEN] = {0};
	char tmpStr[32] = {0};

	/*Inject config port*/
	g_proxy_cfg.config_port = ::GetPrivateProfileInt(_T("Inject"), _T("ConfigPort"), DEF_PROC_COMM_SRV_PORT, _T(g_proxy_cfg_dir));

	/*proxy类型: vpn|server*/	
	::GetPrivateProfileString(_T("Proxy_Basic"), _T("ProxyType"), _T("server"), tmpStr,
								32, _T(g_proxy_cfg_dir));
	g_proxy_cfg.proxy_type = proxy_cfg_get_type(tmpStr);
	if (-1 == g_proxy_cfg.proxy_type)
	{
		strTemp.Format(_T("Read proxy type failed, file %s"), g_proxy_cfg_dir);
		MessageBox(NULL, strTemp, "Load Config failed", MB_OK);
		return -1;
	}

	/*vpn ip*/
	if(0 == ::GetPrivateProfileString(_T("Proxy_Basic"), _T("VpnIp"), _T("0"), ipstr,
								IP_DESC_LEN, _T(g_proxy_cfg_dir)))
	{
		strTemp.Format(_T("Read vpn server ip failed, file %s"), g_proxy_cfg_dir);
		MessageBox(NULL, strTemp, "Load Config failed", MB_OK);
		return -1;
	}
	engine_str_to_ipv4(ipstr, (uint32_t*)&g_proxy_cfg.vpn_ip);
	g_proxy_cfg.vpn_ip = ntohl(g_proxy_cfg.vpn_ip);

	/*vpn port*/
	g_proxy_cfg.vpn_port = ::GetPrivateProfileInt(_T("Proxy_Basic"), _T("VpnPort"), 0, _T(g_proxy_cfg_dir));
	
	/*sock server ip*/
	if(0 == ::GetPrivateProfileString(_T("Proxy_Basic"), _T("SockServerIp"), _T("0"), ipstr,
								IP_DESC_LEN, _T(g_proxy_cfg_dir)))
	{
		strTemp.Format(_T("Read server ip failed, file %s"), g_proxy_cfg_dir);
		MessageBox(NULL, strTemp, "Load Config failed", MB_OK);
		return -1;
	}
	engine_str_to_ipv4(ipstr, (uint32_t*)&g_proxy_cfg.server_ip);
	g_proxy_cfg.server_ip = ntohl(g_proxy_cfg.server_ip);
	
	/*local port*/
	g_proxy_cfg.local_port = ::GetPrivateProfileInt(_T("Proxy_Basic"), _T("LocalPort"), DEF_PROXY_SRV_PORT, _T(g_proxy_cfg_dir));
	/*sock proto*/
	::GetPrivateProfileString(_T("Proxy_Basic"), _T("Proto"), _T("socks4"), tmpStr,
								32, _T(g_proxy_cfg_dir));
	 g_proxy_cfg.proxy_proto = proxy_cfg_get_proto(tmpStr);
	if (-1 == g_proxy_cfg.proxy_proto)
	{
		strTemp.Format(_T("Invalid proxy proto %s"), tmpStr);
		MessageBox(NULL, strTemp, "Load Config failed", MB_OK);
		return -1;
	}

	/*auth*/
	::GetPrivateProfileString(_T("Proxy_Auth"), _T("Username"), _T(""), g_proxy_cfg.username,
								64, _T(g_proxy_cfg_dir));
	::GetPrivateProfileString(_T("Proxy_Auth"), _T("Passwd"), _T(""), g_proxy_cfg.passwd,
							64, _T(g_proxy_cfg_dir));
	int result = ::GetPrivateProfileInt(_T("Proxy_Auth"), _T("IsAuth"), 0, _T(g_proxy_cfg_dir));
	if (result)
	{
		g_proxy_cfg.is_auth = true;
	}
	else
	{
		g_proxy_cfg.is_auth = false;
	}

	/*encry*/
	::GetPrivateProfileString(_T("Proxy_Encry"), _T("EncryType"), _T("none"), tmpStr,
								32, _T(g_proxy_cfg_dir));
	g_proxy_cfg.encry_type = proxy_cfg_get_encry_type(tmpStr);
	if (-1 == g_proxy_cfg.encry_type)
	{
		strTemp.Format(_T("Invalid proxy encry type %s"), tmpStr);
		MessageBox(NULL, strTemp, "Load Config failed", MB_OK);
		return -1;
	}
	::GetPrivateProfileString(_T("Proxy_Encry"), _T("EncryKey"), _T(""), g_proxy_cfg.encry_key,
							sizeof(g_proxy_cfg.encry_key), _T(g_proxy_cfg_dir));

	g_proxy_procs_cnt = ::GetPrivateProfileInt(_T("Process"), _T("Count"), 0, _T(g_proxy_cfg_dir));
	for (int ii = 0; ii < g_proxy_procs_cnt; ii++)
	{
		char nodename[32] = {0};
		SNPRINTF(nodename, 32, "Name_%d", ii+1);
		if(0 == ::GetPrivateProfileString(_T("Process"), _T(nodename), _T(""), g_proxy_procs[ii],
								64, _T(g_proxy_cfg_dir)))
		{
			strTemp.Format(_T("read process %s failed, file %s"),nodename, g_proxy_cfg_dir);
			MessageBox(NULL, strTemp, "Load Config failed", MB_OK);
			return -1;
		}

		_LOG_INFO("get proxy config: process %s", g_proxy_procs[ii]);
	}

	g_proxy_forbit_inject_procs_cnt = ::GetPrivateProfileInt(_T("Forbit_Process"), _T("Count"), 0, _T(g_proxy_cfg_dir));
	for (int ii = 0; ii < g_proxy_forbit_inject_procs_cnt; ii++)
	{
		char nodename[32] = {0};
		SNPRINTF(nodename, 32, "Name_%d", ii+1);
		if(0 == ::GetPrivateProfileString(_T("Forbit_Process"), _T(nodename), _T(""), g_proxy_forbit_inject_procs[ii],
								64, _T(g_proxy_cfg_dir)))
		{
			strTemp.Format(_T("read Forbit_Process %s failed, file %s"),nodename, g_proxy_cfg_dir);
			MessageBox(NULL, strTemp, "Load Config failed", MB_OK);
			return -1;
		}

		_LOG_INFO("get proxy config: Forbit_Process %s", g_proxy_procs[ii]);
	}

	result = ::GetPrivateProfileInt(_T("Status"), _T("IsProxyStart"), 0, _T(g_proxy_cfg_dir));
	if (result)
	{
		g_is_start = true;
	}
	else
	{
		g_is_start = false;
	}

	char logLevelStr[16] = {0};
	::GetPrivateProfileString(_T("Other"), _T("LogLevel"), _T("DEBUG"), logLevelStr,
							sizeof(logLevelStr), _T(g_proxy_cfg_dir));
	if (strncmp(logLevelStr, "DEBUG", strlen("DEBUG")) == 0)
	{
		g_log_level = L_DEBUG;
	}
	else if (strncmp(logLevelStr, "INFO", strlen("INFO")) == 0)
	{
		g_log_level = L_INFO;
	}
	else if (strncmp(logLevelStr, "WARNING", strlen("WARNING")) == 0)
	{
		g_log_level = L_WARN;
	}
	else 
	{
		g_log_level = L_ERROR;
	}

	g_thrd_cnt = ::GetPrivateProfileInt(_T("Other"), _T("ThreadCnt"), 0, _T(g_proxy_cfg_dir));
#endif

	if (-1 == g_log_level)
	{
		g_log_level = L_INFO;
	}

	if (g_thrd_cnt == 0)
	{
		int cpu_num = util_get_cpu_core_num();
		g_thrd_cnt = cpu_num * 2;	
	}
	
    return 0;
}

int proxy_set_loglevel(int loglevel)
{
	g_log_level = loglevel;
	_LOG_INFO("set log level to %d", g_log_level);

	proxy_cfg_save();
	return 0;
}

int proxy_set_thrdcnt(int thrdcnt)
{
	g_thrd_cnt = thrdcnt;
	_LOG_INFO("set thread count to %d", thrdcnt);

	proxy_cfg_save();
	return 0;
}

int proxy_set_status(bool is_start)
{
	g_is_start = is_start;
	_LOG_INFO("%s proxy", is_start?"start":"stop");

	proxy_cfg_save();
	return 0;
}

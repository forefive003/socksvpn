#pragma once
#ifdef _WIN32
#include "common.h"
#endif

#include "engine_ip.h"

/*连接数最大数量*/
#define MAX_REMOTE_SRV_CNT  10

/*默认配置服务端口*/
#define DEF_PROC_COMM_SRV_PORT 33332

typedef enum 
{
    SOCKS_INIT = 1,
    SOCKS_HANDSHAKING,
    SOCKS_AUTHING,
    SOCKS_CONNECTING,
    SOCKS_CONNECTED,
    SOCKS_CONNECTED_FAILED,
}SOCKS_STATUS_E;

#define PROXY_CFG_MAX_PROC 512
#define PROXY_MAX_SERVER  64

enum
{
	PROXY_TYPE_INVALID = 0,
	PROXY_VPN,
	PROXY_SERVER,
};

enum
{
	ENCRY_INVALID = 0,
	ENCRY_NONE,
	ENCRY_XOR,
	ENCRY_RC4,
};

typedef struct 
{
	int proxy_type;
	int proxy_proto;

	char vpn_domain[HOST_DOMAIN_LEN + 1];
	uint32_t vpn_ip;
	uint16_t vpn_port;

	uint32_t  server_ip;
	uint16_t  local_port;
	
	#ifdef _WIN32
	uint16_t  config_port;
	#endif

	bool is_auth;
    char username[64];
    char passwd[64];

    int encry_type;
    char encry_key[64];
}proxy_cfg_t;

int proxy_cfg_get_proto_type(const char* desc);

proxy_cfg_t* proxy_cfg_get();
int proxy_cfg_set(proxy_cfg_t *config);
bool proxy_cfg_is_vpn_type();

int proxy_cfg_add_proc(char *exe_name);
void proxy_cfg_del_proc(char *exe_name);
bool proxy_cfg_proc_is_exist(char *exe_name);
bool proxy_cfg_forbit_inject_proc_is_exist(char *exe_name);

void proxy_set_servers(int *srv_ip_arr, int srv_cnt);

int proxy_cfg_init();
int proxy_cfg_save();

int proxy_set_status(bool is_start);
int proxy_set_loglevel(int loglevel);
int proxy_set_thrdcnt(int thrdcnt);

extern int g_proxy_procs_cnt;
extern char g_proxy_procs[PROXY_CFG_MAX_PROC][64];

/*能够选择的代理服务器*/
extern int g_servers_cnt;
extern char g_servers[PROXY_MAX_SERVER][IP_DESC_LEN];


extern const char* g_proxy_cfg_proto_desc[];
extern const char* g_proxy_cfg_type_desc[];
extern const char* g_proxy_cfg_encry_type_desc[];

extern const char* g_socks_status_desc[];

extern bool g_is_start;

extern int g_log_level;
extern int g_thrd_cnt;
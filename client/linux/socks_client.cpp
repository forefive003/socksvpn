#include "commtype.h"
#include "logproc.h"
#include "socketwrap.h"
#include "CNetAccept.h"
#include "netpool.h"
#include "sigproc.h"
#include "CSocksMem.h"
#include "utilstr.h"
#include "utilcommon.h"

#include "proxyConfig.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"

#include "CLocalServer.h"
#include "CRemoteServer.h"
#include "socks_client.h"
#include "CRemoteServerPool.h"

static BOOL g_exit = false;

static void Usage(char *program)
{
	printf("Usage: params of %s \n", program);
	printf("%-8s -s <socks server ip>\n", "");
	printf("%-8s -l <local port>\n", "");
	printf("%-8s -r <relay server ip>\n", "");
	printf("%-8s -d <relay server domain name>\n", "");
	printf("%-8s -p <relay server port>\n", "");
	printf("%-8s -u <user name>\n", "");
	printf("%-8s -w <password>\n", "");
	printf("%-8s -m <xor|rc4>, Don't take this option when needn't encrypt data\n", "");
	printf("%-8s -e <encrypt key>\n", "");
}

/*
socks_client -s socks_server
	-l local_port 
	-r relay_ip -p relay_port
	-u username -w password 
	-m xor|rc4 -e key
socks_server: socks5服务器IP
local_port: 本地监听端口
relay_ip: 转发服务器IP
relay_port: 转发服务器端口
-m 加密方式
-e 加密密钥
必填参数: socks_server, relay_ip
默认local_port 22222 默认relay_port 22223
*/
static int cmd_parser(proxy_cfg_t *proxy_cfg, int argc, char *argv[])
{
	int opt;
	char tmp_ipstr[HOST_IP_LEN + 1] = {0};

	while ((opt = getopt(argc, argv, "s:l:r:d:p:u:w:m:e:h")) != -1) {
		switch (opt) {
		case 's':
			strncpy(tmp_ipstr, optarg, HOST_IP_LEN);
			if (0 != engine_str_to_ipv4(tmp_ipstr, (uint32_t*)&proxy_cfg->server_ip))
            {
                printf("Invalid relay server %s\n", tmp_ipstr);
                Usage(argv[0]);
                return -1;
            }
            proxy_cfg->server_ip = ntohl(proxy_cfg->server_ip);            
            printf("get option: socks server ip %s\n", tmp_ipstr);
			break;
		case 'l':
			proxy_cfg->local_port = atoi(optarg);
			printf("get option: local port %u\n", proxy_cfg->local_port);
			break;
		case 'r':
			strncpy(tmp_ipstr, optarg, HOST_IP_LEN);
			if (0 != engine_str_to_ipv4(tmp_ipstr, (uint32_t*)&proxy_cfg->vpn_ip))
            {
                printf("Invalid relay server %s\n", tmp_ipstr);
                Usage(argv[0]);
                return -1;
            }
            proxy_cfg->vpn_ip = ntohl(proxy_cfg->vpn_ip);
            printf("get option: relay server ip %s\n", tmp_ipstr);
			break;
		case 'd':
			strncpy(proxy_cfg->vpn_domain, optarg, HOST_DOMAIN_LEN);
			printf("get option: relay server domain %s\n", proxy_cfg->vpn_domain);
			break;
		case 'p':
			proxy_cfg->vpn_port = atoi(optarg);
			printf("get option: relay server port %u\n", proxy_cfg->vpn_port);
			break;
		case 'u':
			strncpy(proxy_cfg->username, optarg, MAX_USERNAME_LEN);
			printf("get option: user name %s\n", proxy_cfg->username);
			break;
		case 'w':
			strncpy(proxy_cfg->passwd, optarg, MAX_PASSWD_LEN);
			printf("get option: password %s\n", proxy_cfg->passwd);
			break;
		case 'm':
			if (!strcmp("xor", optarg))
			{
				proxy_cfg->encry_type = XOR_METHOD;
			}
			else if (!strcmp("rc4", optarg))
			{
				proxy_cfg->encry_type = RC4_METHOD;
			}
			else
			{
				printf("Invalid encrypt method %s", optarg);
				Usage(argv[0]);
				return -1;
			}
			break;
		case 'e':
			strncpy(proxy_cfg->encry_key, optarg, 63);
			break;
		case 'h':
			Usage(argv[0]);
			return -1;
			break;
		default:
			printf("Invalid param key %d", opt);
			Usage(argv[0]);
			return -1;
			break;
		}
	}

	if (proxy_cfg->encry_type != 0)
	{
		if (proxy_cfg->encry_key[0] == 0)
		{
			printf("No encrypt key when use encrypt method\n");
			Usage(argv[0]);
			return -1;
		}
	}

	if (proxy_cfg->vpn_ip == 0 && proxy_cfg->vpn_domain[0] == 0)
	{
		printf("No relay server params\n");
		Usage(argv[0]);
		return -1;
	}
	if (proxy_cfg->server_ip == 0)
	{
		printf("No socks5 server params\n");
		Usage(argv[0]);
		return -1;
	}

    if (proxy_cfg->vpn_domain[0] != 0)
    {
    	printf("get ip for relay server, domain %s\n", proxy_cfg->vpn_domain);
	    if( 0!= util_gethostbyname(proxy_cfg->vpn_domain, &proxy_cfg->vpn_ip))
	    {
	        printf("get domain of relay server failed, domain %s\n", proxy_cfg->vpn_domain);
	        return -1;
	    }
	}

    char ipstr[HOST_IP_LEN] = { 0 };
    engine_ipv4_to_str(htonl(proxy_cfg->vpn_ip), ipstr);
    printf("relay server: %s:%u\n", ipstr, proxy_cfg->vpn_port);

	if (proxy_cfg->username[0] != 0 && proxy_cfg->passwd[0] != 0)
	{
		proxy_cfg->is_auth = true;
	}
	
	return 0;
}


static void stop_process(int sig)
{
	g_exit = true;
    np_let_stop();
    return;
}

static void fatal_process(int sig)
{
	static int mark=0;
	if (mark)
		return;//防止多个信号同时处理

	mark=1;

    write_dumpinfo(sig); /*写入堆栈信息*/
    loggger_exit();
    exit(0);
}

static int register_signal(void)
{
	//sigproc((char*)"socks_client.dump");

	struct sigaction act;

//	create_file(dumpFile);
	strncpy(g_dumpFile, (char*)"socks_client.dump", sizeof(g_dumpFile));

	act.sa_handler = fatal_process;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGSTOP);
    sigaddset(&act.sa_mask, SIGABRT);
    sigaddset(&act.sa_mask, SIGILL);
    sigaddset(&act.sa_mask, SIGTERM);
    //sigaddset(&act.sa_mask, SIGQUIT);
    //sigaddset(&act.sa_mask, SIGINT);
    sigaddset(&act.sa_mask, SIGSEGV);

    sigaction(SIGILL, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    //sigaction(SIGQUIT, &act, NULL);
    //sigaction(SIGINT, &act, NULL);
    sigaction(SIGABRT, &act, NULL);
    sigaction(SIGSTOP, &act, NULL);

    act.sa_handler = stop_process;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGQUIT);
    sigaddset(&act.sa_mask, SIGINT);
    sigaddset(&act.sa_mask, SIGHUP);

    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGHUP, &act, NULL);

    /*忽略SIGPIPE*/
    struct sigaction ign;
	ign.sa_handler = SIG_IGN;
	ign.sa_flags = 0;

	sigemptyset(&ign.sa_mask);
	sigaddset(&ign.sa_mask, SIGPIPE);
	sigaction(SIGPIPE, &ign, NULL);

	return OK;
}

uint64_t g_total_client_cnt = 0;
uint64_t g_total_remote_cnt = 0;
uint64_t g_total_connect_req_cnt = 0;
uint64_t g_total_connect_resp_cnt = 0;
uint64_t g_total_connect_resp_consume_time = 0;


static void print_global_statistic(FILE *pFd)
{
    float speed = 0;
    if (g_total_connect_resp_cnt > 0)
    {
        speed = (g_total_connect_resp_consume_time*1000)/g_total_connect_resp_cnt;
    }

    fprintf(pFd, "GLOBAL-STAT: \n");
    fprintf(pFd, "\ttotal-client: %"PRIu64"\t total-remote:%"PRIu64"\t \n",  
        g_total_client_cnt, g_total_remote_cnt);
    fprintf(pFd, "\ttotal-conn-req: %"PRIu64"\t total-conn-resp:%"PRIu64"\n",  
        g_total_connect_req_cnt, g_total_connect_resp_cnt);
    fprintf(pFd, "\trequest-consume-time: %"PRIu64"\t average-speed(ms/q):%0.2f\n",  
        g_total_connect_resp_consume_time, speed);
    fprintf(pFd, "\ttotal-data-req: %"PRIu64"\t total-data-resp:%"PRIu64"\n",  
        g_total_data_req_cnt, g_total_data_resp_cnt);
    fprintf(pFd, "\n");
}

static void print_statistic()
{
    FILE* pFd = NULL;
    pFd = fopen("/tmp/socks_client.stat", "w");
    if (NULL == pFd) 
    {
       return;
    }
    print_global_statistic(pFd);
    g_ConnMgr->print_statistic(pFd, true);

    fprintf(pFd, "SRVPOOL-STAT:\n");
    g_remoteSrvPool->print_statisc(pFd);

    fclose(pFd);
    return;
}
static void _timer_callback(void* param1, void* param2,
                void* param3, void* param4)
{
	static uint32_t expire_cnt = 0;
	expire_cnt++;

    if (expire_cnt % 10 == 0)
    {
        print_statistic();
    }
	
	g_remoteSrvPool->status_check();    
}

int main(int argc, char **argv)
{
	proxy_cfg_t proxy_cfg;
	memset(&proxy_cfg, 0, sizeof(proxy_cfg));
	proxy_cfg.proxy_type = PROXY_VPN;
	proxy_cfg.proxy_proto = SOCKS_5;
	proxy_cfg.local_port = DEF_CLIENT_PORT;
    proxy_cfg.vpn_port = DEF_RELAY_CLI_PORT;

	if (0 != cmd_parser(&proxy_cfg, argc, argv))
    {
        return -1;
    }
    proxy_cfg_set(&proxy_cfg);

	loggger_init((char*)("/tmp/"), (char*)("socks_client"), 1 * 1024, 6, true);
	logger_set_level(L_INFO);

	register_signal();

	if (FALSE == np_init())
	{
		printf("netsock init failed.\n");
		loggger_exit();
		return -1;
	}
	int cpu_num = util_get_cpu_core_num();
	np_init_worker_thrds(cpu_num*2);
	
	if (0 != socks_mem_init(CLIENT_MEM_NODE_CNT))
	{
		printf("socks mem init failed.\n");
		loggger_exit();
		return -1;
	}

	g_ConnMgr = CConnMgr::instance();
	g_remoteSrvPool = new CRemoteServerPool(MAX_REMOTE_SRV_CNT);
    g_remoteSrvPool->init();

	g_LocalServ = CLocalServer::instance(proxy_cfg.local_port);
	if(OK != g_LocalServ->init())
	{
		printf("local server init failed.\n");
		loggger_exit();
		return -1;
	}

	MUTEX_SETUP(g_remote_srv_lock);

    /*start check timer*/
    np_add_time_job(_timer_callback, NULL, NULL, NULL, NULL, 1, FALSE);

	np_start();
	while(g_exit == false)
    {
        sleep(1);
    }

	np_wait_stop();
	MUTEX_CLEANUP(g_remote_srv_lock);
	loggger_exit();
	return 0;
}

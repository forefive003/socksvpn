#include "commtype.h"
#include "logproc.h"
#include "socketwrap.h"
#include "CNetRecv.h"
#include "netpool.h"
#include "relay_pkt_def.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CLocalServer.h"
#include "socks_server.h"
#include "sigproc.h"
#include "CSocksMem.h"
#include "utilfile.h"

char g_relay_ipstr[HOST_IP_LEN + 1] = {0};
char g_relay_domain[HOST_DOMAIN_LEN + 1] = {0};
uint32_t g_relay_ip = 0;
uint16_t g_relay_port = DEF_RELAY_SRV_PORT;

encry_key_t *g_encry_key = NULL;
int g_encry_method = 0;

char g_server_sn[MAX_SN_LEN] = {0};

static void Usage(char *program)
{
    printf("Usage: params of %s \n", program);
    printf("%-8s -h for help\n", "");
    printf("%-8s -r <relay server ip>\n", "");
    printf("%-8s -d <relay server domain name>\n", "");
    printf("%-8s -p <relay server port>\n", "");
    printf("%-8s -m <xor|rc4>, Don't take this option when needn't encrypt data\n", "");
    printf("%-8s -e <encrypt key>\n", "");
    printf("%-8s -n <the sn of server, max 32 bytes len, for example: AABBCCDD002233bb>\n", "");
}


/*
socks_server -m xor|rc4 -e key -r relay_ip -p relay_port
-m 加密方式
-e 加密密钥
relay_ip: 转发服务器IP
relay_port: 转发服务器端口
必填参数: relay_ip
默认relay_port 22225
*/
static int cmd_parser(int argc, char *argv[])
{
    int opt;
    struct sockaddr_storage ipaddr;
    size_t key_len;

    while ((opt = getopt(argc, argv, "hr:d:p:m:e:n:")) != -1) {
        switch (opt) {
        case 'h':
            Usage(argv[0]);
            return -1;
            break;
        case 'n':
            strncpy(g_server_sn, optarg, MAX_SN_LEN-1);
            printf("get option: serverSN is %s\n", g_server_sn);
            break;

        case 'r':
            strncpy(g_relay_ipstr, optarg, HOST_IP_LEN);
            if (0 != util_str_to_ip(g_relay_ipstr, &ipaddr))
            {
                printf("Invalid relay server %s\n", g_relay_ipstr);
                Usage(argv[0]);
                return -1;
            }
            g_relay_ip = htonl(((struct sockaddr_in *)&ipaddr)->sin_addr.s_addr);
            printf("get option: relay server ip %s\n", g_relay_ipstr);
            break;
        case 'd':
            strncpy(g_relay_domain, optarg, HOST_DOMAIN_LEN);
            printf("get option: relay server domain %s\n", g_relay_domain);
            break;
        case 'p':
            g_relay_port = atoi(optarg);
            printf("get option: relay server port %u\n", g_relay_port);
            break;
        case 'm':
            if (!strcmp("xor", optarg))
            {
                g_encry_method = XOR_METHOD;
            }
            else if (!strcmp("rc4", optarg))
            {
                g_encry_method = RC4_METHOD;
            }
            else
            {
                printf("Invalid encrypt method %s", optarg);
                Usage(argv[0]);
                return -1;
            }
            break;
        case 'e':
            key_len = strlen(optarg);
            g_encry_key = (encry_key_t*)malloc(sizeof(encry_key_t) + key_len);
            g_encry_key->len = key_len;
            memcpy(g_encry_key->key, optarg, key_len);
            break;
        default:
            printf("Invalid param key %d", opt);
            Usage(argv[0]);
            return -1;
            break;
        }
    }

    if (g_encry_method != 0)
    {
        if (NULL == g_encry_key)
        {
            printf("No encrypt key when use encrypt method\n");
            return -1;
        }
    }


    if (g_relay_ip == 0 && g_relay_domain[0] == 0)
    {
        printf("No relay server params\n");
        Usage(argv[0]);
        return -1;
    }
    return 0;
}

static BOOL g_exit = false;

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

//  create_file(dumpFile);
    strncpy(g_dumpFile, (char*)"socks_server.dump", sizeof(g_dumpFile));

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

    fprintf(pFd, "GLOBAL-STAT:\n");
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
    pFd = fopen("/tmp/socks_server.stat", "w");
    if (NULL == pFd) 
    {
       return;
    }
    print_global_statistic(pFd);
    g_ConnMgr->print_statistic(pFd, false);

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

    MUTEX_LOCK(m_local_srv_lock);
    if (NULL == g_LocalServ)
    {
        g_LocalServ = new CLocalServer(g_relay_ip, g_relay_port);
        g_LocalServ->init_async_write_resource(socks_malloc, socks_free);
        if(0 != g_LocalServ->init())
        {
            delete g_LocalServ;
            g_LocalServ = NULL;
        }
    }
    else
    {
        if (g_LocalServ->is_connected())
        {
            if (expire_cnt % 10 == 0)
            {
                g_LocalServ->send_keepalive();
            }
        }
    }
    MUTEX_UNLOCK(m_local_srv_lock);
}

int main(int argc, char **argv)
{
    if (0 != cmd_parser(argc, argv))
    {
        return -1;
    }

    if (g_relay_domain[0] != 0)
    {
        printf("get ip for relay server, domain %s\n", g_relay_domain);
        if( 0!= util_gethostbyname(g_relay_domain, &g_relay_ip))
        {
            printf("get domain of relay server failed, domain %s\n", g_relay_domain);
            return -1;
        }
    }

    struct sockaddr_storage ipaddr;
    char ipstr[64] = { 0 };
    memset((void*) &ipaddr, 0, sizeof(struct sockaddr_storage));
    ipaddr.ss_family = AF_INET;
    ((struct sockaddr_in *) &ipaddr)->sin_addr.s_addr = htonl(g_relay_ip);
    if (NULL == util_ip_to_str(&ipaddr, ipstr)) 
    {
        printf("ip to str failed for relay server.\n");
        return -1;
    }
    strncpy(g_relay_ipstr, ipstr, HOST_IP_LEN);
    printf("relay server: %s:%u\n", g_relay_ipstr, g_relay_port);
    
    register_signal();

	loggger_init((char*)("/tmp/"), (char*)("socks_server"), 1 * 1024, 6, true);
	logger_set_level(L_INFO);

	if (FALSE == np_init())
	{
		printf("netsock init failed.\n");
        loggger_exit();
		return -1;
	}
    int cpu_num = util_get_cpu_core_num();
    np_init_worker_thrds(cpu_num*2);
    
    if (0 != socks_mem_init(SERVER_MEM_NODE_CNT))
    {
        printf("socks mem init failed.\n");
        loggger_exit();
        return -1;
    }

#ifndef _WIN32
    pthread_mutexattr_t mux_attr;
    memset(&mux_attr, 0, sizeof(mux_attr));
    pthread_mutexattr_settype(&mux_attr, PTHREAD_MUTEX_RECURSIVE);
    MUTEX_SETUP_ATTR(m_local_srv_lock, &mux_attr);
#else
    MUTEX_SETUP_ATTR(m_local_srv_lock, NULL);
#endif

    g_ConnMgr = CConnMgr::instance();

    /*start check timer*/
    np_add_time_job(_timer_callback, NULL, NULL, NULL, NULL, 1, FALSE);

    np_start();
    while(g_exit == false)
    {
        sleep(1);
    }
    np_wait_stop();

    loggger_exit();
	return 0;
}

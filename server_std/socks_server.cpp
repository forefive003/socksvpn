#include "commtype.h"
#include "logproc.h"
#include "socketwrap.h"
#include "CNetRecv.h"
#include "CNetAccept.h"
#include "netpool.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CLocalServer.h"
#include "socks_server.h"
#include "sigproc.h"
#include "CSocksMem.h"
#include "utilcommon.h"


uint16_t g_local_port = DEF_CLIENT_PORT;

static void Usage(char *program)
{
    printf("Usage: params of %s \n", program);
    printf("%-8s -l <local port>\n", "");
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

    while ((opt = getopt(argc, argv, "l:m:e:h")) != -1) {
        switch (opt) {
        case 'l':
            g_local_port = atoi(optarg);
            printf("get option: local server port %u\n", g_local_port);
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
    g_ConnMgr->print_statistic(pFd, true);

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
}

int main(int argc, char **argv)
{
    if (0 != cmd_parser(argc, argv))
    {
        return -1;
    }
    
    register_signal();

	loggger_init((char*)("/tmp/"), (char*)("socks_server_std"), 1 * 1024, 6, true);
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

    g_ConnMgr = CConnMgr::instance();

    g_LocalServ = CLocalServer::instance(g_local_port);
    if(OK != g_LocalServ->init())
    {
        printf("local server init failed.\n");
        loggger_exit();
        return -1;
    }

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

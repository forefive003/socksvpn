#include <getopt.h>
#include "commtype.h"
#include "logproc.h"
#include "socketwrap.h"
#include "common_def.h"
#include "sigproc.h"
#include "utilfile.h"

#define DEF_CONFIG_SRV_PORT 22226
#define MAX_REQ_MSG_LEN  1048576 //1024 * 1024
#define MAX_RESP_MSG_LEN  1048576

static int g_send_fd = -1;
static char g_config_srv[256] = {0};
static int g_config_port = DEF_CONFIG_SRV_PORT;

static char g_req_buf[MAX_REQ_MSG_LEN] = {0};
static char g_resp_buf[MAX_RESP_MSG_LEN] = {0};


static int send_request(char *buf, int buf_len)
{
    if (g_send_fd == -1)
    {
        g_send_fd = sock_connect(g_config_srv, g_config_port);
        if (-1 == g_send_fd)
        {
            printf("failed to connect to server %s, port %d\n", g_config_srv, g_config_port);
            return -1;
        }
    }

    printf("send req: %s, len %d\n", buf, buf_len);
    if ( buf_len != sock_write_timeout(g_send_fd, buf, buf_len, 3))
    {
        char err_buf[64] = {0};
        printf("send failed, %s.\n", str_error_s(err_buf, 32, errno));
        close(g_send_fd);
        g_send_fd = -1;
        return -1;
    }

    int recv_len = 0;
    int ret_len = 0;
    while(recv_len < MAX_RESP_MSG_LEN)
    {
        ret_len = sock_read_timeout(g_send_fd, &g_resp_buf[recv_len], 1, 6);
        if (ret_len == 0)
        {
            char err_buf[64] = {0};
            printf("recv failed, %s.\n", str_error_s(err_buf, 32, errno));
            close(g_send_fd);
            g_send_fd = -1;
            return -1;
        }

        recv_len += ret_len;
        if (g_resp_buf[recv_len-1] == 0)
        {
            break;
        }
    }

    printf("get resp: %s\n", g_resp_buf);
    return 0;
}

static int set_srv_config(char *srv_ip, char *pri_ip)
{
    int ret_len = 0;

    ret_len = snprintf(g_req_buf, 512, "{\"relaysn\":\"relay_testsn\",\"password\":\"123456\",\"body\": {");
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "\"sn\":\"server_testsn\", \"pub-ip\":\"%s\", \"pri-ip\":\"%s\", \"users\":[",
        srv_ip, pri_ip);

    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "{\"username\":\"%s\", \"passwd\":\"%s\", \"enabled\":1},",
        "user1","passwd1");
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "{\"username\":\"%s\", \"passwd\":\"%s\", \"enabled\":1},",
        "user2","passwd2");
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "{\"username\":\"%s\", \"passwd\":\"%s\", \"enabled\":1},",
        "user3","passwd3");
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "{\"username\":\"%s\", \"passwd\":\"%s\", \"enabled\":1},",
        "user4","passwd4");
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "{\"username\":\"%s\", \"passwd\":\"%s\", \"enabled\":1}",
        "user5","passwd5");

    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "]}}");

    return send_request(g_req_buf, ret_len+1);
}

/*
request: 
    "type":"set-debug",
    "level":"debug|info|warn"
response: "code":0
*/
static int set_debug_level(char *debug_level)
{
    int ret_len = 0;

    ret_len = snprintf(g_req_buf, 512, "{\"type\":\"set-debug\", \"level\":\"%s\"}", debug_level);
    return send_request(g_req_buf, ret_len+1);
}

/*
request: 
    "type":"get-server-cfg",
response: "code":0, "params":"..."
*/
static int get_all_srv()
{
    int ret_len = 0;

    ret_len = snprintf(g_req_buf, 512, "{\"type\":\"set-get-server-cfg\"}");
    return send_request(g_req_buf, ret_len+1);
}

static void Usage(char *program)
{
    printf("Usage: params of %s \n", program);
    printf("%-8s --relay x.x.x.x --port xxx\n", "");
    printf("%-8s --set --srv-ip x.x.x.x --pri-ip x.x.x.x\n", "");
    printf("%-8s --set --debug info|debug|warn\n", "");
    printf("%-8s --get --all-srv\n", "");
}

static struct option g_long_options[] =
{
        {"set",  no_argument,       NULL, 's'},
        {"get",  no_argument,       NULL, 'g'},
        {"srv-ip",  required_argument, NULL, 'r'},
        {"pri-ip",  required_argument, NULL, 'p'},
        {"debug", required_argument, NULL, 'd'},
        {"all-srv", no_argument, NULL, 'a'},
        {"relay", required_argument, NULL, 'l'},
        {"port", required_argument, NULL, 't'},
        {0, 0, 0, 0}
};

const static char *g_short_opts = "sgr:p:d:al:t:";

int main(int argc, char **argv)
{
    int c;
    int option_index = 0;

    bool has_set_opt = false;

    bool is_set = false;
    char *srv_ip = NULL;
    char *pri_ip = NULL;

    char *debug_level = NULL;
    bool is_get_all = false;

    opterr = 0;
    optind = 1;
    while ( (c = getopt_long(argc, argv, g_short_opts, g_long_options, &option_index)) != -1 )
    {
        switch ( c )
        {
        case 's':
            is_set = true;
            has_set_opt = true;
            break;
        case 'g':
            is_set = false;
            has_set_opt = true;
            break;
        case 'r':
            srv_ip = optarg;
            break;
        case 'p':
            pri_ip = optarg;
            break;
        case 'd':
            debug_level = optarg;
            break;
        case 'a':
            is_get_all = true;
            break;
        case 'l':
            strncpy(g_config_srv, optarg, 256);
            break;
        case 't':
            g_config_port = atoi(optarg);
            break;
        default:
            printf("Invalid param key %d\n", c);
            Usage(argv[0]);
            return -1;
            break;
        }
    }

    if (g_config_srv[0] == 0)
    {
        printf("must give relay server ipaddr option\n");
        Usage(argv[0]);
        return -1;
    }

    if (has_set_opt == false)
    {
        printf("must give set or get option\n");
        Usage(argv[0]);
        return -1;
    }

    if (is_set)
    {
        if (NULL != srv_ip && NULL != pri_ip)
        {
            set_srv_config(srv_ip, pri_ip);
        }
        else if (debug_level != NULL)
        {
            set_debug_level(debug_level);
        }
        else
        {
            printf("must give srv_ip end pri_ip option\n");
            Usage(argv[0]);
            return -1;
        }
    }
    else
    {
        if (is_get_all)
        {
            get_all_srv();
        }
    }

    return 0;
}

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
            printf("get end flag\n");
            break;
        }
    }

    printf("get resp: %s\n", g_resp_buf);
    return 0;
}

static int set_srv_user_pass(char *srv_ip, char *pri_ip, char *username, char *passwd)
{
    int ret_len = 0;

    ret_len = snprintf(g_req_buf, 512, "{\"operation\":\"set\",\"target\":\"socks-srv\",\"param\": [");
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "{\"pub-ip\":\"%s\", \"pri-ip\":\"%s\", \"username\":\"%s\", \"passwd\":\"%s\"}",
        srv_ip, pri_ip, username, passwd);
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "]}");

    return send_request(g_req_buf, ret_len+1);
}

static int set_srv_enabled(char *srv_ip, char *pri_ip, int enabled)
{
    int ret_len = 0;

    ret_len = snprintf(g_req_buf, 512, "{\"operation\":\"set\",\"target\":\"socks-srv\",\"param\": [");
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "{\"pub-ip\":\"%s\", \"pri-ip\":\"%s\", \"enabled\":%d}",
        srv_ip, pri_ip, enabled);
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "]}");

    return send_request(g_req_buf, ret_len+1);
}

static int set_debug_level(char *debug_level)
{
    int ret_len = 0;

    ret_len = snprintf(g_req_buf, 512, "{\"operation\":\"set\",\"target\":\"debug\",\"param\": ");
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "{\"level\":\"%s\"}", debug_level);
    ret_len += snprintf(&g_req_buf[ret_len], 512-ret_len, "}");

    return send_request(g_req_buf, ret_len+1);
}

static int get_all_srv()
{
    int ret_len = 0;

    ret_len = snprintf(g_req_buf, 512, "{\"operation\":\"get\",\"target\":\"socks-srv\",\"param\": \"all\"}");
    return send_request(g_req_buf, ret_len+1);
}

static void Usage(char *program)
{
    printf("Usage: params of %s \n", program);
    printf("%-8s --relay x.x.x.x --port xxx\n", "");
    printf("%-8s --set --srv-ip x.x.x.x --pri-ip x.x.x.x --usrname xxxxx --passwd xxxxx --enable 1|0\n", "");
    printf("%-8s --set --debug info|debug|warn\n", "");
    printf("%-8s --get --all-srv\n", "");
}

static struct option g_long_options[] =
{
        {"set",  no_argument,       NULL, 's'},
        {"get",  no_argument,       NULL, 'g'},
        {"srv-ip",  required_argument, NULL, 'r'},
        {"pri-ip",  required_argument, NULL, 'p'},
        {"usrname", required_argument, NULL, 'u'},
        {"passwd", required_argument, NULL, 'w'},
        {"enable", required_argument, NULL, 'e'},
        {"debug", required_argument, NULL, 'd'},
        {"all-srv", no_argument, NULL, 'a'},
        {"relay", required_argument, NULL, 'l'},
        {"port", required_argument, NULL, 't'},
        {0, 0, 0, 0}
};

const static char *g_short_opts = "sgr:p:u:w:e:d:al:t:";

/*
socks_config --set --srv-ip x.x.x.x --pri-ip x.x.x.x --usrname xxxxx --passwd xxxxx --enable 1|0
             --set --debug info|debug|warn
             --get --all-srv
*/
int main(int argc, char **argv)
{
    int c;
    int option_index = 0;

    bool has_enable_opt = false;
    bool has_set_opt = false;

    bool is_set = false;
    char *srv_ip = NULL;
    char *pri_ip = NULL;
    char *usrname = NULL;
    char *passwd = NULL;
    int enabled = 0;

    char *debug_level = NULL;
    char *get_val = NULL;

    opterr = 0;
    optind = 1;
    while ( (c = getopt_long(argc, argv, g_short_opts, g_long_options, &option_index)) != -1 )
    {
        switch ( c )
        {
        case 's':
            printf("fff\n");
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
        case 'u':
            usrname = optarg;
            break;
        case 'w':
            passwd = optarg;
            break;
        case 'e':
            enabled = atoi(optarg);
            has_enable_opt = true;
            break;
        case 'd':
            debug_level = optarg;
            break;
        case 'a':
            get_val = optarg;
            get_val = get_val;
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
            if (NULL != usrname && NULL != passwd)
            {
                set_srv_user_pass(srv_ip, pri_ip, usrname, passwd);
            }

            if (has_enable_opt)
            {
                set_srv_enabled(srv_ip, pri_ip, enabled);
            }
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
        get_all_srv();
    }

    return 0;
}

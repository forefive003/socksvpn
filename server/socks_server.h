
#ifndef _SOCKS5_SERVER_H
#define _SOCKS5_SERVER_H

#define MAX_LOCAL_SRV_CNT  10

extern char g_relay_ipstr[HOST_IP_LEN + 1];
extern char g_relay_domain[HOST_DOMAIN_LEN + 1];
extern uint32_t g_relay_ip;
extern uint16_t g_relay_port;

extern encry_key_t *g_encry_key;
extern int g_encry_method;


extern uint64_t g_total_client_cnt;
extern uint64_t g_total_remote_cnt;
extern uint64_t g_total_connect_req_cnt;
extern uint64_t g_total_connect_resp_cnt;
extern uint64_t g_total_connect_resp_consume_time;

extern char g_server_sn[MAX_SN_LEN];

#endif

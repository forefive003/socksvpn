#ifndef _SOCKS5_CLIENT_H
#define _SOCKS5_CLIENT_H

extern char g_relay_ipstr[HOST_IP_LEN + 1];
extern char g_relay_domain[HOST_DOMAIN_LEN + 1];
extern uint32_t g_relay_ip;
extern uint16_t g_relay_port;

extern char g_server_ipstr[HOST_IP_LEN + 1];
extern uint32_t g_server_ip;

extern uint16_t g_local_port;

extern char g_username[MAX_USERNAME_LEN + 1];
extern char g_password[MAX_PASSWD_LEN + 1];

extern encry_key_t *g_encry_key;
extern int g_encry_method;


extern const char* g_cli_status_desc[];


extern uint64_t g_total_client_cnt;
extern uint64_t g_total_remote_cnt;
extern uint64_t g_total_connect_req_cnt;
extern uint64_t g_total_connect_resp_cnt;
extern uint64_t g_total_connect_resp_consume_time;

#endif

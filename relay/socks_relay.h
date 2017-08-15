#ifndef _SOCKS5_RELAY_H
#define _SOCKS5_RELAY_H


#define  MAX_URL_LEN 256

BOOL is_relay_need_auth();

extern char g_relaysn[MAX_SN_LEN];
extern char g_relay_passwd[MAX_PASSWD_LEN];
extern char g_relay_url[MAX_URL_LEN];

extern uint64_t g_new_client_cnt;
extern uint64_t g_new_remote_cnt;

extern uint64_t g_total_client_cnt;
extern uint64_t g_total_remote_cnt;
extern uint64_t g_total_connect_req_cnt;
extern uint64_t g_total_connect_resp_cnt;
extern uint64_t g_total_connect_resp_consume_time;

#endif

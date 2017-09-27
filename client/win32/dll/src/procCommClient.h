#pragma once

#include "commtype.h"

#pragma warning( disable : 4200 ) 
/*默认配置服务端口*/
#define DEF_PROC_COMM_SRV_PORT 33332

enum
{
	MSG_REG_REQ = 0,
	MSG_REG_RESP,

	MSG_UNREG_REQ,

	MSG_LOG,

	MSG_MAX
};

typedef struct 
{
	uint64_t  proc_id;
}reg_req_body_t;

typedef struct 
{
	uint64_t  proc_id;
}unreg_req_body_t;

typedef struct 
{
	uint32_t  local_port;
}reg_resp_body_t;

typedef struct 
{
	uint16_t type;
	uint16_t len;
}proc_msg_t;

int proc_send_unreg_req();
int proc_send_reg_req(reg_resp_body_t *resp_data);
int proc_send_log(char *log_buf, int log_len);

#pragma once

#include "CNetAccept.h"
#include "CNetRecv.h"

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

#if 0
int proc_set_params(HANDLE hPipe, char *param_buf, int param_len);
HANDLE proc_comm_init(DWORD dwPid);
void proc_comm_free(HANDLE hPipe);
#endif


class CProcMsgServer : public CNetAccept
{
public:   
	CProcMsgServer(uint16_t port) : CNetAccept(port)
	{
		_LOG_INFO("construct ProcMsgServer on port %u", m_listen_port);
	}
	virtual ~CProcMsgServer()
    {
        _LOG_INFO("destruct ProcMsgServer on port %u", m_listen_port);
    }

private:
    int accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port);
};

int proc_comm_init();
void proc_comm_free();


#pragma once

#include "hookapi.h"
#include "logproc.h"


enum
{
	HOOK_API_CONNECT = 0,
	HOOK_API_WSA_CONNECT,
	HOOK_API_CONNECT_EX,

	HOOK_API_SEND,
	HOOK_API_WSA_SEND,
	HOOK_API_WSA_SENDMSG,

	//HOOK_API_WSA_IOCTL,

	HOOK_API_DISCONNECT_EX,
	HOOK_API_CLOSE_SOCKET,

	HOOK_API_MAX
};

extern HANDLE g_process_hdl;
extern HMODULE g_local_module;
extern HOOKAPI g_hook_api[HOOK_API_MAX];


extern DWORD  g_cur_proc_pid;
extern char  g_cur_proc_name[256];

extern unsigned int g_local_ipaddr;
extern unsigned short g_local_port;


int injectInit();
void injectFree();
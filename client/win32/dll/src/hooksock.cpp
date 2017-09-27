#include "stdafx.h"
#include <windows.h> 
#include <Winsock2.h>

#include "procCommClient.h"
#include "hooksock.h"
#include "entry.h"
#include "proxysocks.h"
#include "socketwrap.h"


int origin_sock_write_timeout(int fd, char *buf, int len, int second)
{
    int n = 0;
    int send_len = len;
    char err_buf[64] = {0};

    int errno_ll = 0;

    while (len)
    {
        int ret = sock_writeable(fd, second);
        if (1 == ret)
        {
            n = ((LPFN_sock_send)(g_hook_api[HOOK_API_SEND].pOldProc))(fd, buf, len, 0);
            if (n < 0)
            {
#ifdef _WIN32
                errno_ll = WSAGetLastError();
                if (errno_ll == WSAEWOULDBLOCK)
                    continue;
#else
                errno_ll = errno;
                if (errno_ll == EINTR && errno_ll == EWOULDBLOCK && errno_ll == EAGAIN)
                    continue;
#endif      
                printf("safe_write failed to write %ld bytes, %s.\n",
                    (long)len, str_error_s(err_buf, 32, errno_ll));
                return 0;
            }

            buf += n;
            len -= n;
        }
        else
        {
            break;
        }
    }

    return send_len - len;
}

int WINAPI hook_sock_connect(
	_In_ SOCKET                s,
	_In_ const struct sockaddr *name,
	_In_ int                   namelen
)  
{
    if (!HookAPIResume(&g_hook_api[HOOK_API_CONNECT]))
    {
        SetLastError(WSAECONNREFUSED);
        return SOCKET_ERROR;
    }

	int lasterr = 0;
    int ret = 0;
    SOCKADDR_IN *real_name = (SOCKADDR_IN*)name;
    uint32_t real_serv_ip = ntohl(real_name->sin_addr.s_addr);
    uint16_t real_serv_port = ntohs(real_name->sin_port);
    if (real_serv_ip == 0x7f000001
            || real_serv_ip == 0x00
            || (real_serv_ip == g_local_ipaddr && real_serv_port == g_local_port))
    {
        ret = ((LPFN_sock_connect)(g_hook_api[HOOK_API_CONNECT].pOldProc))(s, name, namelen);
		lasterr = GetLastError();
    }
    else
    {
        if(0 != proxysocks_connect_handle((int)s, real_serv_ip, real_serv_port))
        {
            HookAPIRecovery(&g_hook_api[HOOK_API_CONNECT]); 
            SetLastError(WSAECONNREFUSED);
            return SOCKET_ERROR;
        }

        SOCKADDR_IN proxy_addr;
        memset(&proxy_addr, 0, sizeof(proxy_addr));
    	proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_addr.s_addr = htonl(g_local_ipaddr);
        proxy_addr.sin_port = htons(g_local_port);

        ret = ((LPFN_sock_connect)(g_hook_api[HOOK_API_CONNECT].pOldProc))(s, (struct sockaddr*)&proxy_addr, namelen);
		lasterr = GetLastError();
	}

    HookAPIRecovery(&g_hook_api[HOOK_API_CONNECT]);    
    SetLastError(lasterr);
    return ret;
}

int WINAPI hook_sock_WSAConnect(
	_In_  SOCKET                s,
	_In_  const struct sockaddr *name,
	_In_  int                   namelen,
	_In_  LPWSABUF              lpCallerData,
	_Out_ LPWSABUF              lpCalleeData,
	_In_  LPQOS                 lpSQOS,
	_In_  LPQOS                 lpGQOS
)
{
    if (!HookAPIResume(&g_hook_api[HOOK_API_WSA_CONNECT]))
    {
        SetLastError(WSAECONNREFUSED);
        return SOCKET_ERROR;
    }

	int lasterr = 0;
    int ret = 0;
    SOCKADDR_IN *real_name = (SOCKADDR_IN*)name;
    uint32_t real_serv_ip = ntohl(real_name->sin_addr.s_addr);
    uint16_t real_serv_port = ntohs(real_name->sin_port);
    if (real_serv_ip == 0x7f000001
            || real_serv_ip == 0x00
            || (real_serv_ip == g_local_ipaddr && real_serv_port == g_local_port))
    {
        ret = ((LPFN_sock_WSAConnect)g_hook_api[HOOK_API_WSA_CONNECT].pOldProc)(s, name, namelen,
            lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
		lasterr = GetLastError();
    }
    else
    {
		if (0 != proxysocks_connect_handle((int)s, real_serv_ip, real_serv_port))
        {
            HookAPIRecovery(&g_hook_api[HOOK_API_WSA_CONNECT]);
            SetLastError(WSAECONNREFUSED);
            return SOCKET_ERROR;
        }
        
		SOCKADDR_IN proxy_addr;
		memset(&proxy_addr, 0, sizeof(proxy_addr));
		proxy_addr.sin_family = AF_INET;
		proxy_addr.sin_addr.s_addr = htonl(g_local_ipaddr);
		proxy_addr.sin_port = htons(g_local_port);

        ret = ((LPFN_sock_WSAConnect)g_hook_api[HOOK_API_WSA_CONNECT].pOldProc)(s, (struct sockaddr*)&proxy_addr, namelen,
        	lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
		lasterr = GetLastError();
    }

    HookAPIRecovery(&g_hook_api[HOOK_API_WSA_CONNECT]);    
    SetLastError(lasterr);
    return ret;
}

BOOL WINAPI hook_sock_ConnectEx(
    _In_     SOCKET                s,
    _In_     const struct sockaddr *name,
    _In_     int                   namelen,
    _In_opt_ PVOID                 lpSendBuffer,
    _In_     DWORD                 dwSendDataLength,
    _Out_    LPDWORD               lpdwBytesSent,
    _In_     LPOVERLAPPED          lpOverlapped
)
{    
    if (!HookAPIResume(&g_hook_api[HOOK_API_CONNECT_EX]))
    {
        SetLastError(WSAECONNREFUSED);
        return false;
    }

	int lasterr = 0;
    BOOL ret = 0;
    SOCKADDR_IN *real_name = (SOCKADDR_IN*)name;
    uint32_t real_serv_ip = ntohl(real_name->sin_addr.s_addr);
    uint16_t real_serv_port = ntohs(real_name->sin_port);
    if (real_serv_ip == 0x7f000001
            || real_serv_ip == 0x00
            || (real_serv_ip == g_local_ipaddr && real_serv_port == g_local_port))
    {
        ret = ((LPFN_sock_ConnectEx)g_hook_api[HOOK_API_CONNECT_EX].pOldProc)(s, name, namelen,
            lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);    
		lasterr = GetLastError();
    }
    else
    {
		if (0 != proxysocks_connect_handle((int)s, real_serv_ip, real_serv_port))
        {
            HookAPIRecovery(&g_hook_api[HOOK_API_CONNECT_EX]);
            SetLastError(WSAECONNREFUSED);
            return SOCKET_ERROR;
        }

        SOCKADDR_IN proxy_addr;
        memset(&proxy_addr, 0, sizeof(proxy_addr));
    	proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_addr.s_addr = htonl(g_local_ipaddr);
        proxy_addr.sin_port = htons(g_local_port);

    	ret = ((LPFN_sock_ConnectEx)g_hook_api[HOOK_API_CONNECT_EX].pOldProc)(s, (struct sockaddr *)&proxy_addr, namelen,
        	lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
		lasterr = GetLastError();
    }
		
    HookAPIRecovery(&g_hook_api[HOOK_API_CONNECT_EX]);    
	SetLastError(lasterr);
	return ret;
}

int WINAPI hook_sock_send(
    _In_       SOCKET s,
    _In_ const char   *buf,
    _In_       int    len,
    _In_       int    flags
)
{
    if (!HookAPIResume(&g_hook_api[HOOK_API_SEND]))
    {
        SetLastError(WSAEHOSTUNREACH);
        return SOCKET_ERROR;
    }

	if (0 != proxysocks_send_handle((int)s, len))
    {
        HookAPIRecovery(&g_hook_api[HOOK_API_SEND]);
        SetLastError(WSAEHOSTUNREACH);         
        return SOCKET_ERROR;
    }

    int ret = 0;
	ret = ((LPFN_sock_send)(g_hook_api[HOOK_API_SEND].pOldProc))(s, buf, len, flags);

    int lasterr = GetLastError();
    HookAPIRecovery(&g_hook_api[HOOK_API_SEND]);    
    SetLastError(lasterr);
    return ret;
}

int WINAPI hook_sock_WSASend(
    _In_  SOCKET                             s,
    _In_  LPWSABUF                           lpBuffers,
    _In_  DWORD                              dwBufferCount,
    _Out_ LPDWORD                            lpNumberOfBytesSent,
    _In_  DWORD                              dwFlags,
    _In_  LPWSAOVERLAPPED                    lpOverlapped,
    _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    if (!HookAPIResume(&g_hook_api[HOOK_API_WSA_SEND]))
    {
        SetLastError(WSAEHOSTUNREACH);
        return SOCKET_ERROR;
    }

	//TODO:
	if (0 != proxysocks_send_handle((int)s, dwBufferCount))
    {
        HookAPIRecovery(&g_hook_api[HOOK_API_WSA_SEND]); 
        SetLastError(WSAEHOSTUNREACH);
        return SOCKET_ERROR;
    }

    int ret = 0;
    ret = ((LPFN_sock_WSASend)g_hook_api[HOOK_API_WSA_SEND].pOldProc)(s, lpBuffers, dwBufferCount,
        lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);

    int lasterr = GetLastError();
    HookAPIRecovery(&g_hook_api[HOOK_API_WSA_SEND]);    
    SetLastError(lasterr);
    return ret;
}

int WINAPI hook_sock_WSASendMsg(
  _In_  SOCKET                             s,
  _In_  LPWSAMSG                           lpMsg,
  _In_  DWORD                              dwFlags,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    if (!HookAPIResume(&g_hook_api[HOOK_API_WSA_SENDMSG]))
    {
        SetLastError(WSAEHOSTUNREACH);
        return false;
    }

	//_LOG_INFO("TEST, WSAsendMsg");
	///TODO:
	if (0 != proxysocks_send_handle((int)s, 1))
    {
        HookAPIRecovery(&g_hook_api[HOOK_API_WSA_SENDMSG]);
        SetLastError(WSAEHOSTUNREACH);
        return SOCKET_ERROR;
    }


    int ret = ((LPFN_sock_WSASendMsg)g_hook_api[HOOK_API_WSA_SENDMSG].pOldProc)(s, lpMsg, 
        dwFlags, lpNumberOfBytesSent, lpOverlapped, lpCompletionRoutine);

    int lasterr = GetLastError();
    HookAPIRecovery(&g_hook_api[HOOK_API_WSA_SENDMSG]);    
    SetLastError(lasterr);
    return ret;
}

BOOL WINAPI hook_sock_DisconnectEx(
    _In_ SOCKET       hSocket,
    _In_ LPOVERLAPPED lpOverlapped,
    _In_ DWORD        dwFlags,
    _In_ DWORD        reserved
)
{
    if (!HookAPIResume(&g_hook_api[HOOK_API_DISCONNECT_EX]))
    {
        SetLastError(WSAEHOSTUNREACH);
        return false;
    }

	_LOG_INFO("TEST, Disconnect");
	proxysocks_close_handle((int)hSocket);

    int ret = ((LPFN_sock_DisconnectEx)g_hook_api[HOOK_API_DISCONNECT_EX].pOldProc)(hSocket,
        lpOverlapped, dwFlags, reserved);

    int lasterr = GetLastError();
    HookAPIRecovery(&g_hook_api[HOOK_API_DISCONNECT_EX]);    
    SetLastError(lasterr);
    return ret;       
}

int WINAPI hook_sock_closesocket(
  _In_ SOCKET s
)
{
    if (!HookAPIResume(&g_hook_api[HOOK_API_CLOSE_SOCKET]))
    {
        SetLastError(WSAEHOSTUNREACH);
        return false;
    }

	//_LOG_INFO("TEST, close");
	proxysocks_close_handle((int)s);

    int ret = ((LPFN_sock_closesocket)g_hook_api[HOOK_API_CLOSE_SOCKET].pOldProc)(s);

    int lasterr = GetLastError();
    HookAPIRecovery(&g_hook_api[HOOK_API_CLOSE_SOCKET]);    
    SetLastError(lasterr);
    return ret;    
}

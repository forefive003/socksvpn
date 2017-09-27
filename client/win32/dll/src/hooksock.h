#pragma once

typedef int (WINAPI *LPFN_sock_connect)(
	_In_ SOCKET                s,
	_In_ const struct sockaddr *name,
	_In_ int                   namelen
);

typedef int (WINAPI *LPFN_sock_send)(
	_In_       SOCKET s,
	_In_ const char   *buf,
	_In_       int    len,
	_In_       int    flags
);

typedef int (WINAPI *LPFN_sock_WSAConnect)(
	SOCKET                s,
	const struct sockaddr *name,
	int                   namelen,
	LPWSABUF              lpCallerData,
	LPWSABUF              lpCalleeData,
	LPQOS                 lpSQOS,
	LPQOS                 lpGQOS
);
typedef BOOL (WINAPI *LPFN_sock_ConnectEx)(
	_In_     SOCKET                s,
	_In_     const struct sockaddr *name,
	_In_     int                   namelen,
	_In_opt_ PVOID                 lpSendBuffer,
	_In_     DWORD                 dwSendDataLength,
	_Out_    LPDWORD               lpdwBytesSent,
	_In_     LPOVERLAPPED          lpOverlapped
);
typedef BOOL (WINAPI *LPFN_sock_DisconnectEx)(
	_In_ SOCKET       hSocket,
	_In_ LPOVERLAPPED lpOverlapped,
	_In_ DWORD        dwFlags,
	_In_ DWORD        reserved
);

typedef int (WINAPI *LPFN_sock_WSASend)(
	_In_  SOCKET                             s,
	_In_  LPWSABUF                           lpBuffers,
	_In_  DWORD                              dwBufferCount,
	_Out_ LPDWORD                            lpNumberOfBytesSent,
	_In_  DWORD                              dwFlags,
	_In_  LPWSAOVERLAPPED                    lpOverlapped,
	_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef int (WINAPI *LPFN_sock_WSASendMsg)(
  _In_  SOCKET                             s,
  _In_  LPWSAMSG                           lpMsg,
  _In_  DWORD                              dwFlags,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);


typedef int (WINAPI *LPFN_sock_closesocket)(
  _In_ SOCKET s
);

/***********************************************************************/
int WINAPI hook_sock_connect(
	_In_ SOCKET                s,
	_In_ const struct sockaddr *name,
	_In_ int                   namelen
);

int WINAPI hook_sock_WSAConnect(
	SOCKET                s,
	const struct sockaddr *name,
	int                   namelen,
	LPWSABUF              lpCallerData,
	LPWSABUF              lpCalleeData,
	LPQOS                 lpSQOS,
	LPQOS                 lpGQOS
);

BOOL WINAPI hook_sock_ConnectEx(
	_In_     SOCKET                s,
	_In_     const struct sockaddr *name,
	_In_     int                   namelen,
	_In_opt_ PVOID                 lpSendBuffer,
	_In_     DWORD                 dwSendDataLength,
	_Out_    LPDWORD               lpdwBytesSent,
	_In_     LPOVERLAPPED          lpOverlapped
	);
BOOL WINAPI hook_sock_DisconnectEx(
    _In_ SOCKET       hSocket,
    _In_ LPOVERLAPPED lpOverlapped,
    _In_ DWORD        dwFlags,
    _In_ DWORD        reserved
);

int WINAPI hook_sock_send(
    _In_       SOCKET s,
    _In_ const char   *buf,
    _In_       int    len,
    _In_       int    flags
);


int WINAPI hook_sock_WSASend(
    _In_  SOCKET                             s,
    _In_  LPWSABUF                           lpBuffers,
    _In_  DWORD                              dwBufferCount,
    _Out_ LPDWORD                            lpNumberOfBytesSent,
    _In_  DWORD                              dwFlags,
    _In_  LPWSAOVERLAPPED                    lpOverlapped,
    _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

int WINAPI hook_sock_WSASendMsg(
  _In_  SOCKET                             s,
  _In_  LPWSAMSG                           lpMsg,
  _In_  DWORD                              dwFlags,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

int WINAPI hook_sock_closesocket(
  _In_ SOCKET s
);


int origin_sock_write_timeout(int fd, char *buf, int len, int second);
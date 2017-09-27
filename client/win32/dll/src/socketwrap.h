
#ifndef SOCKET_WRAP_H_
#define SOCKET_WRAP_H_


#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include "commtype.h"

#ifdef __cplusplus
extern "C"
{
#endif



enum
{
	RECV_OK = 0,
	RECV_ERROR,
	RECV_TIMEOUT,
};

#define BIGPATHBUFLEN (1024)

DLL_API bool sock_set_block(int fd);
DLL_API bool sock_set_unblock(int fd);
DLL_API int sock_readable(int fd, int second);
DLL_API int sock_writeable(int fd, int second);
DLL_API int sock_read_timeout(int fd, char *buf, int len, int second);
DLL_API int sock_write_timeout(int fd, char *buf, int len, int second);
DLL_API int sock_send_line(int fd, const char *format, ...);
DLL_API int sock_recv_line(int fd, char *buf, size_t bufsiz, int eof_ok);

DLL_API int sock_create_server(uint16_t port);
DLL_API int sock_accept(int s, unsigned int *client_ip, uint16_t *client_port);
DLL_API int sock_connect(const char *addr, uint16_t port);
DLL_API void sock_close(int fd);

DLL_API int util_gethostbyname(char *domain, uint32_t *ipaddr);

#ifdef __cplusplus
}
#endif



#endif


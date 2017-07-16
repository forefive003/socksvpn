
#ifndef SOCKET_WRAP_H_
#define SOCKET_WRAP_H_


#ifdef _WIN32
#include <windows.h>

#ifdef ETP_DLL_EXPORT
#define DLL_API __declspec(dllexport)
#elif defined(ETP_USE_DLL)
#define DLL_API __declspec(dllimport)
#elif defined(ETP_USE_LIB)
#define DLL_API 
#else
#define DLL_API 
#endif

#else
#include <unistd.h>
#include <sys/stat.h>
#define DLL_API extern
#endif


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

DLL_API BOOL sock_set_block(int fd);
DLL_API BOOL sock_set_unblock(int fd);
DLL_API int sock_readable(int fd, int second);
DLL_API int sock_writeable(int fd, int second);
DLL_API int sock_read_timeout(int fd, char *buf, size_t len, int second);
DLL_API int sock_write_timeout(int fd, char *buf, size_t len, int second);
DLL_API int sock_send_line(int fd, const char *format, ...);
DLL_API int sock_recv_line(int fd, char *buf, size_t bufsiz, int eof_ok);

DLL_API int sock_create_server(uint16_t port);
DLL_API int sock_accept(int s, unsigned int *client_ip, uint16_t *client_port);
DLL_API int sock_connect(const char *addr, uint16_t port);

DLL_API int util_gethostbyname(char *domain, uint32_t *ipaddr);

#ifdef __cplusplus
}
#endif



#endif


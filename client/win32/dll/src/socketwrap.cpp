
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>


#ifdef _WIN32
#include <Ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "commtype.h"
#include "socketwrap.h"
#include "win_inet_pton.h"


DLL_API bool sock_set_block(int fd)
{
#ifndef _WIN32
    int flags = fcntl(fd,F_GETFL,0);
    flags &= ~O_NONBLOCK;
    fcntl(fd,F_SETFL,flags);
#else
    u_long iMode = 0;  //non-blocking mode is enabled.
    ioctlsocket(fd, FIONBIO, &iMode);
#endif
    return true;
}

DLL_API bool sock_set_unblock(int fd)
{
#ifndef _WIN32
    int flags = fcntl(fd,F_GETFL,0);
    flags |= O_NONBLOCK;
    fcntl(fd,F_SETFL,flags);
#else
    u_long iMode = 1;  //non-blocking mode is enabled.
    ioctlsocket(fd, FIONBIO, &iMode);
#endif
    return true;
}

DLL_API int sock_readable(int fd, int second)
{
    struct timeval tv;
    fd_set r_fds, e_fds;
    int cnt;
    char err_buf[64] = {0};

nextcheck:
    FD_ZERO(&r_fds);
    FD_SET(fd, &r_fds);

    FD_ZERO(&e_fds);
    FD_SET(fd, &e_fds);

    tv.tv_sec = second;
    tv.tv_usec = 0;

    int errno_ll = 0;
    if (second != -1)
    {
        cnt = select(fd+1, &r_fds, NULL, &e_fds, &tv);
    }
    else
    {
        cnt = select(fd+1, &r_fds, NULL, &e_fds, NULL);
    }
    if (cnt < 0)
    {
#ifdef _WIN32 
        errno_ll = WSAGetLastError();
        if (errno_ll == WSAEBADF)
        {
            printf("check_read return -1, select failed [%s]\n", str_error_s(err_buf, 32, errno_ll));
            return -1;
        }
        else if (errno_ll == WSAEINTR || errno_ll == WSAEWOULDBLOCK)
        {
            printf("check_read select meet intr, continue\n");
            goto nextcheck;
        }
#else
        errno_ll = errno;
        if (errno_ll == EBADF)
        {
            printf("check_read return -1, select failed [%s]\n", str_error_s(err_buf, 32, errno_ll));
            return -1;
        }
        else if (errno_ll == EINTR)
        {
            printf("check_read select meet intr, continue\n");
            goto nextcheck;
        }
        else if (errno_ll == EWOULDBLOCK || errno_ll == EAGAIN)
        {
            printf("check_read select would block, continue\n");
            goto nextcheck;
        }
#endif
        printf("check_read return 0, select failed [%s]\n", str_error_s(err_buf, 32, errno_ll));
        return 0;
    }
    else if (0 == cnt)
    {
        /*timeout, not recved*/
        printf("check_read select timeout [%d]\n",second);
        return 0;
    }

    if (FD_ISSET(fd, &r_fds))
    {
        return 1;
    }

#ifdef _WIN32
    errno_ll = WSAGetLastError();
#else
    errno_ll = errno;
#endif

    printf("check_read return 0, select no fd,  [%s]\n", str_error_s(err_buf, 32, errno_ll));
    return 0;
}

DLL_API int sock_writeable(int fd, int second)
{
    fd_set w_fds; 
    struct timeval tv;
    int cnt = 0;
    char err_buf[64] = {0};

    int errno_ll = 0;

nextcheck:
    FD_ZERO(&w_fds);
    FD_SET(fd, &w_fds);

    tv.tv_sec = second;
    tv.tv_usec = 0;

    if (second != -1)
    {
        cnt = select(fd + 1, NULL, &w_fds, NULL, &tv);
    }
    else
    {
        cnt = select(fd + 1, NULL, &w_fds, NULL, NULL);
    }
    if (cnt < 0)
    {
#ifdef _WIN32 
        errno_ll = WSAGetLastError();
        if (errno_ll == WSAEBADF)
        {
            printf("check_write return -1, select failed [%s]\n", str_error_s(err_buf, 32, errno_ll));
            return -1;
        }
        else if (errno_ll == WSAEINTR || errno_ll == WSAEWOULDBLOCK)
        {
            printf("check_write select meet intr, continue\n");
            goto nextcheck;
        }
#else
        errno_ll = errno;
        if (errno_ll == EBADF)
        {
            printf("check_write return -1, select failed [%s]\n", str_error_s(err_buf, 32, errno_ll));
            return -1;
        }
        else if (errno_ll == EINTR)
        {
            printf("check_write select meet intr, continue\n");
            goto nextcheck;
        }
        else if (errno_ll == EWOULDBLOCK || errno_ll == EAGAIN)
        {
            printf("check_write select would block, continue\n");
            goto nextcheck;
        }
#endif
        printf("check_write return 0, select failed [%s]\n", str_error_s(err_buf, 32, errno_ll));
        return 0;
    }
    else if (0 == cnt)
    {
        /*timeout, not recved*/
        printf("check_write select timeout [%d]\n",second);
        return 0;
    }

    if (FD_ISSET(fd, &w_fds))
    {
        return 1;
    }

#ifdef _WIN32
    errno_ll = WSAGetLastError();
#else
    errno_ll = errno;
#endif

    printf("check_write return 0, select no fd,  [%s]\n", str_error_s(err_buf, 32, errno_ll));
    return 0;
}

DLL_API int sock_read_timeout(int fd, char *buf, int len, int second)
{
    int got = 0;
    char err_buf[64] = {0};

    int errno_ll = 0;

    while (1)
    {
        int ret = sock_readable(fd, second);
        if (1 == ret)
        {
            int n = recv(fd, buf + got, len - got, 0);
            if (n == 0)
            {
                printf("safe_read peer closed.\n");
                got = 0;
                break;
            }

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

                printf("safe_read failed to read %ld bytes, %s\n",
                        (long)len, str_error_s(err_buf, 32, errno_ll));
                return 0;
            }

            if ((got += n) == len)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    return got;
}

DLL_API int sock_write_timeout(int fd, char *buf, int len, int second)
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
            n = send(fd, buf, len, 0);
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

DLL_API int sock_send_line(int fd, const char *format, ...)
{
    va_list ap;
    char buf[BIGPATHBUFLEN];
    int len;

    va_start(ap, format);
    len = VSNPRINTF(buf, BIGPATHBUFLEN, format, ap);
    va_end(ap);

    if (len < 0 || len > BIGPATHBUFLEN)
    {
        return ERROR;
    }

    return (len == sock_read_timeout(fd, buf, len, 3)) ? 0 : ERROR;
}

/* Read a line of up to bufsiz-1 characters into buf.  Strips
 * the (required) trailing newline and all carriage returns.
 * Returns 1 for success; 0 for I/O error or truncation. */
DLL_API int sock_recv_line(int fd, char *buf, size_t bufsiz, int eof_ok)
{
    bufsiz--; /* leave room for the null */
    while (bufsiz > 0)
    {
        if (sock_read_timeout(fd, buf, 1, 3) != 1)
        {
            return RECV_ERROR;
        }

        if (*buf == '\0')
            return RECV_OK;

        if (*buf == '\n')
            break;

        if (*buf != '\r')
        {
            buf++;
            bufsiz--;
        }
    }

    *buf = '\0';

    return (bufsiz > 0 ? RECV_OK : RECV_ERROR);
}


static int _bind_server(int server_s, char *server_ip, uint16_t server_port)
{
    struct sockaddr_in server_sockaddr;

    memset(&server_sockaddr, 0, sizeof server_sockaddr);
    server_sockaddr.sin_family = AF_INET;
	if (server_ip != NULL)
		win_inet_pton(AF_INET, server_ip, &server_sockaddr.sin_addr);
    else
        server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons(server_port);
    return bind(server_s, (struct sockaddr *) &server_sockaddr,
            sizeof(server_sockaddr));
}

DLL_API int sock_create_server(uint16_t port)
{
    int server_s;
    int sock_opt = 1;
    int ret;

	server_s = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_s == -1)
    {
#ifndef _WIN32
        printf("unable to create socket, %s\n", strerror(errno));
#else
        printf("unable to create socket, %d\n", WSAGetLastError());
#endif
        return -1;
    }

    /* accept fd must set noblock for rst */
    ret = sock_set_unblock(server_s);
    if (ret < 0)
    {
        printf("sock_set_unblock failed\n");
        return -1;
    }

    if ((setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, (char*)&sock_opt, sizeof(sock_opt))) == -1)
    {
#ifndef _WIN32
        printf("setsockopt failed, %s\n", strerror(errno));
#else
        printf("setsockopt failed, %d\n", WSAGetLastError());
#endif
        
        return -1;
    }

    /* internet family-specific code encapsulated in bind_server()  */
    if (_bind_server(server_s, NULL, port) == -1)
    {
#ifndef _WIN32
        printf("unable to bind,%s\n", strerror(errno));
#else
        printf("unable to bind,%d\n", WSAGetLastError());
#endif
        return -1;
    }

    listen(server_s, 100);
    return server_s;
}

DLL_API int sock_accept(int s, unsigned int *client_ip, uint16_t *client_port)
{
    int fd;
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);

	if ((fd = (int)accept(s, (struct sockaddr *)&sa, &salen)) < 0)
    {
        return -1;
    }
    
    if (client_ip)
    {
        *client_ip = ntohl(sa.sin_addr.s_addr);
    }

    if (client_port)
    {
        *client_port = ntohs(sa.sin_port);
    }
    return fd;
}

DLL_API int sock_connect(const char *addr, uint16_t port)
{
    int s;
    struct sockaddr_in sa;
	
	if ((s = (int)socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		int error = WSAGetLastError();
		return -1;
	}

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
		if (win_inet_pton(AF_INET, addr, &sa.sin_addr) == 0) {
        struct hostent *he;

        /*
         * FIXME: it may block!
         * not support IPv6. getarrrinfo() is better.
         */
        he = gethostbyname(addr);
        if (he == NULL) {
            
#ifdef _WIN32
            printf("can't resolve: %s:%u %d\n", addr, port, WSAGetLastError());
			closesocket(s);
#else
            printf("can't resolve: %s:%u %s\n", addr, port, strerror(errno));
            close(s);
#endif
            return -1;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }

    /*set to nonblock*/
    sock_set_unblock(s);

    /* FIXME: it may block! */
    if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) == 0) {
		/*set to block*/
		sock_set_block(s);

        return s;
    }
    
#ifdef _WIN32
    int errno_ll = WSAGetLastError();
    if (errno_ll != WSAEWOULDBLOCK && errno_ll != WSAEALREADY)
    {
        printf("connect failed, %s:%u %d\n", addr, port, errno_ll);
		closesocket(s);
        return -1;
    }
#else
    if (errno != EINPROGRESS)
    {
        printf("connect failed, %s:%u %s\n", addr, port, strerror(errno));
        close(s);
        return -1;
    }
#endif

    /*wait*/
    struct timeval tv;
    fd_set w_fds;
    int cnt = 0;

    FD_ZERO(&w_fds);
    FD_SET(s, &w_fds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    cnt = select(s + 1, NULL, &w_fds, NULL, &tv);
    if (cnt < 0)
    {
#ifdef _WIN32
        printf("connect failed, %s:%u %d\n", addr, port, WSAGetLastError());
		closesocket(s);
#else
        printf("connect failed, %s: %s\n", addr, strerror(errno));
		close(s);
#endif
        return -1;
    }
    else if (0 == cnt)
    {
        /*timeout, not recved*/
        printf("connect %s timeout\n", addr);
#ifdef _WIN32
		closesocket(s);
#else
		close(s);
#endif
        return -1;
    }

    if (FD_ISSET(s, &w_fds))
    {
        int err = -1;
        socklen_t len = sizeof(int);
        if ( getsockopt(s,  SOL_SOCKET, SO_ERROR ,(char*)&err, &len) < 0 )
        {
            
#ifdef _WIN32
            printf("connect failed, %s: %d\n", addr, WSAGetLastError());
			closesocket(s);
#else
            printf("connect failed, %s: %s\n", addr, strerror(errno));
			close(s);
#endif
            return -1;
        }

        if (err)
        {
            
#ifdef _WIN32
            printf("connect failed, %s: %d\n", addr, WSAGetLastError());
			closesocket(s);
#else
            printf("connect failed, %s: %s\n", addr, strerror(err));
			close(s);
#endif
            return -1;
        }

        /*set to block*/
        sock_set_block(s);
        return s;
    }

    /*timeout, not recved*/
    
#ifdef _WIN32
    printf("connect failed, %s: %d\n", addr, WSAGetLastError());
	closesocket(s);
#else
    printf("connect failed, %s: %s\n", addr, strerror(errno));
	close(s);
#endif
    return -1;
}


DLL_API void sock_close(int fd)
{
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

DLL_API int util_gethostbyname(char *domain, uint32_t *ipaddr)
{
    struct hostent *hptr = NULL;
    char **pptr;

    if ((hptr = gethostbyname(domain)) == NULL) 
    {
#ifdef _WIN32
        printf("gethostbyname %s failed: %d\n", domain, WSAGetLastError());
#else
        printf("gethostbyname %s failed: %s\n", domain, strerror(errno));
#endif
        return -1;
    }

    if (hptr->h_addrtype == AF_INET) 
    {
        pptr = hptr->h_addr_list;
        for (; *pptr != NULL; pptr++) 
        {
            struct in_addr *tempaddr = (struct in_addr *)*pptr;
            *ipaddr = ntohl(tempaddr->s_addr);
            //break;
            //printf("get ipaddr 0x%x, domain %s\n", *ipaddr, domain);
        }
    }
    else if (hptr->h_addrtype == AF_INET6) 
    {
        ///TODO:
        return -1;
    }
    else
    {
        printf("invalid addrtype %d\n", hptr->h_addrtype);
        return -1;
    }

    return 0;
}
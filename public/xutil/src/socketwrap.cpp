
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "commtype.h"
#include "socketwrap.h"

DLL_API BOOL sock_set_block(int fd)
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

DLL_API BOOL sock_set_unblock(int fd)
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

    cnt = select(fd+1, &r_fds, NULL, &e_fds, &tv);
    if (cnt < 0)
    {
        if (errno == EBADF)
        {
            
            printf("check_read return -1, select failed [%s]\n", str_error_s(err_buf, 32, errno));
            return -1;
        }
        else if (errno == EINTR)
        {
            printf("check_read select meet intr, continue\n");
            goto nextcheck;
        }

        printf("check_read return 0, select failed [%s]\n", str_error_s(err_buf, 32, errno));
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

    printf("check_read return 0, select no fd,  [%s]\n", str_error_s(err_buf, 32, errno));
    return 0;
}

DLL_API int sock_writeable(int fd, int second)
{
    fd_set w_fds; 
    struct timeval tv;
    int cnt = 0;
    char err_buf[64] = {0};

nextcheck:
    FD_ZERO(&w_fds);
    FD_SET(fd, &w_fds);

    tv.tv_sec = second;
    tv.tv_usec = 0;

    cnt = select(fd + 1, NULL, &w_fds, NULL, &tv);
    if (cnt < 0)
    {
        if (errno == EBADF)
        {
            printf("check_write select -1, failed [%s]\n", str_error_s(err_buf, 32, errno));
            return -1;
        }
        else if (errno == EINTR)
        {
            printf("check_write select meet intr, continue\n");
            goto nextcheck;
        }

        printf("check_write return 0, select failed [%s]\n", str_error_s(err_buf, 32, errno));
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

    printf("check_write return 0, select no fd,  [%s]\n", str_error_s(err_buf, 32, errno));
    return 0;
}

DLL_API int sock_read_timeout(int fd, char *buf, size_t len, int second)
{
    size_t got = 0;
    char err_buf[64] = {0};

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
                if (errno == EINTR && errno == EWOULDBLOCK && errno == EAGAIN)
                    continue;

                printf("safe_read failed to read %ld bytes, %s\n",
                        (long)len, str_error_s(err_buf, 32, errno));
                return 0;
            }

            if ((got += (size_t)n) == len)
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

DLL_API int sock_write_timeout(int fd, char *buf, size_t len, int second)
{
    int n = 0;
    int send_len = len;
    char err_buf[64] = {0};

    while (len)
    {
        int ret = sock_writeable(fd, second);
        if (1 == ret)
        {
            n = send(fd, buf, len, 0);
            if (n < 0)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    printf("safe_write failed to write %ld bytes, %s.\n",
                        (long)len, str_error_s(err_buf, 32, errno));
                    return 0;
                }
                else
                {
                    continue;
                }
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
        inet_aton(server_ip, &server_sockaddr.sin_addr);
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

    server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_s == -1)
    {
        printf("unable to create socket, %s\n", strerror(errno));
        return -1;
    }

    /* accept fd must set noblock for rst */
    ret = sock_set_unblock(server_s);
    if (ret < 0)
    {
        printf("sock_set_unblock failed\n");
        return -1;
    }

    /* close server socket on exec so cgi's can't write to it */
    if (fcntl(server_s, F_SETFD, 1) == -1)
    {
        printf("can't set close-on-exec, %s\n", strerror(errno));
        return -1;
    }

    if ((setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, (void *)&sock_opt, sizeof(sock_opt))) == -1)
    {
        printf("setsockopt failed, %s\n", strerror(errno));
        return -1;
    }

    /* internet family-specific code encapsulated in bind_server()  */
    if (_bind_server(server_s, NULL, port) == -1)
    {
        printf("unable to bind, %s\n", strerror(errno));
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

    if ((fd = accept(s, (struct sockaddr *)&sa, &salen)) < 0)
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

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_aton(addr, &sa.sin_addr) == 0) {
        struct hostent *he;

        /*
         * FIXME: it may block!
         * not support IPv6. getarrrinfo() is better.
         */
        he = gethostbyname(addr);
        if (he == NULL) {
            printf("can't resolve: %s:%u %s\n", addr, port, strerror(errno));
            close(s);
            return -1;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }

    /*set to nonblock*/
    sock_set_unblock(s);

    /* FIXME: it may block! */
    if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) == 0) {
        return s;
    }
    
    if (errno != EINPROGRESS)
    {
        printf("connect failed, %s:%u %s\n", addr, port, strerror(errno));
        close(s);
        return -1;
    }

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
        printf("connect failed, %s: %s\n", addr, strerror(errno));
        close(s);
        return -1;
    }
    else if (0 == cnt)
    {
        /*timeout, not recved*/
        printf("connect %s timeout\n", addr);
        close(s);
        return -1;
    }

    if (FD_ISSET(s, &w_fds))
    {
        int err = -1;
        socklen_t len = sizeof(int);
        if ( getsockopt(s,  SOL_SOCKET, SO_ERROR ,&err, &len) < 0 )
        {
            printf("connect failed, %s: %s\n", addr, strerror(errno));
            close(s);
            return -1;
        }

        if (err)
        {
            printf("connect failed, %s: %s\n", addr, strerror(err));
            close(s);
            return -1;
        }

        /*set to block*/
        sock_set_block(s);
        return s;
    }

    /*timeout, not recved*/
    printf("connect failed, %s: %s\n", addr, strerror(errno));
    close(s);
    return -1;
}


DLL_API int util_gethostbyname(char *domain, uint32_t *ipaddr)
{
    struct hostent *hptr = NULL;
    char **pptr;

    if ((hptr = gethostbyname(domain)) == NULL) 
    {
        printf("gethostbyname %s failed: %s\n", domain, strerror(errno));
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
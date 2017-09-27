
#ifndef _WIN_INET_NTOP_H_
#define _WIN_INET_NTOP_H_

#ifdef _WIN32

extern const char * win_inet_ntop(int af, const void *src, char *dst, size_t size);

#endif

#endif

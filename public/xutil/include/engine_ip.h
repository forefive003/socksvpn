#ifndef _ENGINE_IP_H
#define _ENGINE_IP_H

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <Ws2tcpip.h>
#endif

#include "commtype.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*
1. IPv4: struct sockaddr_in, 16 个字节

 struct sockaddr_in {
      sa_family_t sin_family;             // AF_INET 
      in_port_t sin_port;                 // Port number.  
      struct in_addr sin_addr;            // Internet address.  
 
      // Pad to size of `struct sockaddr'.  
      unsigned char sin_zero[sizeof (struct sockaddr) -
                             sizeof (sa_family_t) -
                             sizeof (in_port_t) -
                             sizeof (struct in_addr)];
 };
 typedef uint32_t in_addr_t;
 struct in_addr  {
     in_addr_t s_addr;                    // IPv4 address
 };
2. IPv6: struct sockaddr_in6, 28 个字节

 struct sockaddr_in6 {
     sa_family_t sin6_family;    // AF_INET6 
     in_port_t sin6_port;        // Transport layer port # 
     uint32_t sin6_flowinfo;     // IPv6 flow information 
     struct in6_addr sin6_addr;  // IPv6 address 
     uint32_t sin6_scope_id;     // IPv6 scope-id 
 };
 struct in6_addr {
     union {
         uint8_t u6_addr8[16];
         uint16_t u6_addr16[8];
         uint32_t u6_addr32[4];
     } in6_u;
 
     #define s6_addr                 in6_u.u6_addr8
     #define s6_addr16               in6_u.u6_addr16
     #define s6_addr32               in6_u.u6_addr32
 };
3. 通用结构体1: struct sockaddr, 16个字节

 struct sockaddr { 
      sa_family_t sa_family;       // Address family 
      char sa_data[14];            // protocol-specific address 
 };
4. 通用结构体2: struct sockaddr_storage,128个字节

 // Structure large enough to hold any socket address 
 (with the historical exception of AF_UNIX). 128 bytes reserved.  
 
 #if ULONG_MAX > 0xffffffff
 # define __ss_aligntype __uint64_t
 #else
 # define __ss_aligntype __uint32_t
 #endif
 #define _SS_SIZE        128
 #define _SS_PADSIZE     (_SS_SIZE - (2 * sizeof (__ss_aligntype)))
 
 struct sockaddr_storage
 {
     sa_family_t ss_family;      // Address family 
     __ss_aligntype __ss_align;  // Force desired alignment.  
     char __ss_padding[_SS_PADSIZE];
 };
*/



#define IP_DESC_LEN  46 //INET6_ADDRSTRLEN
#define HOST_IP_LEN  IP_DESC_LEN
#define HOST_DOMAIN_LEN 256

#define NIP4_FMT "%u.%u.%u.%u"
#define NIP6_FMT "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x"

enum ENGINE_ADDR_TYPE
{
    ADDRTYPE_UNKOWN=0,
    ADDRTYPE_IPV4=1,
    ADDRTYPE_IPV6=2,
};

typedef struct engine_ip
{
    int type;
    union
    {
        unsigned int _v4;
        struct in6_addr _v6;
    } ip_union;
} engine_ip_t;

#ifndef _WIN32
#define IPV6_ADDR_COPY(daddr, saddr)  \
    (daddr)->s6_addr32[0] = (saddr)->s6_addr32[0]; \
    (daddr)->s6_addr32[1] = (saddr)->s6_addr32[1]; \
    (daddr)->s6_addr32[2] = (saddr)->s6_addr32[2]; \
    (daddr)->s6_addr32[3] = (saddr)->s6_addr32[3];

#define IPV6_ADDR_COPY_NET(daddr, saddr)  \
    (daddr)->s6_addr32[0] = htonl((saddr)->s6_addr32[0]); \
    (daddr)->s6_addr32[1] = htonl((saddr)->s6_addr32[1]); \
    (daddr)->s6_addr32[2] = htonl((saddr)->s6_addr32[2]); \
    (daddr)->s6_addr32[3] = htonl((saddr)->s6_addr32[3]);

#define IPV6_ADDR_EQUAL_COMMON(daddr,saddr)  \
    ((daddr)->s6_addr32[0] == (saddr)->s6_addr32[0] \
      && (daddr)->s6_addr32[1] == (saddr)->s6_addr32[1] \
      && (daddr)->s6_addr32[2] == (saddr)->s6_addr32[2] \
      && (daddr)->s6_addr32[3] == (saddr)->s6_addr32[3])

static inline void engine_ipv6_to_host(struct in6_addr *ipv6)
{
    ipv6->s6_addr32[0] = ntohl(ipv6->s6_addr32[0]);
    ipv6->s6_addr32[1] = ntohl(ipv6->s6_addr32[1]);
    ipv6->s6_addr32[2] = ntohl(ipv6->s6_addr32[2]);
    ipv6->s6_addr32[3] = ntohl(ipv6->s6_addr32[3]);
}

static inline void engine_ipv6_to_net(struct in6_addr *ipv6)
{
	ipv6->s6_addr32[0] = ntohl(ipv6->s6_addr32[0]);
	ipv6->s6_addr32[1] = ntohl(ipv6->s6_addr32[1]);
	ipv6->s6_addr32[2] = ntohl(ipv6->s6_addr32[2]);
	ipv6->s6_addr32[3] = ntohl(ipv6->s6_addr32[3]);
}

#else
#define IPV6_ADDR_COPY(daddr, saddr)  \
	uint32_t *daddr32 = (uint32_t*)&(daddr)->u.Byte[0];\
	uint32_t *saddr32 = (uint32_t*)&(saddr)->u.Byte[0];\
    daddr32[0] = saddr32[0]; \
    daddr32[1] = saddr32[1]; \
    daddr32[2] = saddr32[2]; \
    daddr32[3] = saddr32[3];

#define IPV6_ADDR_COPY_NET(daddr, saddr)  \
    uint32_t *daddr32 = (uint32_t*)&(daddr)->u.Byte[0];\
	uint32_t *saddr32 = (uint32_t*)&(saddr)->u.Byte[0];\
    daddr32[0] = htonl(saddr32[0]); \
    daddr32[1] = htonl(saddr32[1]); \
    daddr32[2] = htonl(saddr32[2]); \
    daddr32[3] = htonl(saddr32[3]);

static inline bool IPV6_ADDR_EQUAL_COMMON(struct in6_addr *daddr, struct in6_addr *saddr)
{
	uint32_t *daddr32 = (uint32_t*)&(daddr)->u.Byte[0]; 
	uint32_t *saddr32 = (uint32_t*)&(saddr)->u.Byte[0]; 
	
	return (daddr32[0] == saddr32[0]
		&& daddr32[1] == saddr32[1]
		&& daddr32[2] == saddr32[2]
		&& daddr32[3] == saddr32[3]);
}

static inline void engine_ipv6_to_host(struct in6_addr *ipv6)
{
	uint32_t *addr32 = (uint32_t*)&ipv6->u.Byte[0];
	addr32[0] = ntohl(addr32[0]);
	addr32[1] = ntohl(addr32[1]);
	addr32[2] = ntohl(addr32[2]);
	addr32[3] = ntohl(addr32[3]);
}

static inline void engine_ipv6_to_net(struct in6_addr *ipv6)
{
	uint32_t *addr32 = (uint32_t*)&ipv6->u.Byte[0];
	addr32[0] = htonl(addr32[0]);
	addr32[1] = htonl(addr32[1]);
	addr32[2] = htonl(addr32[2]);
	addr32[3] = htonl(addr32[3]);
}
#endif

static inline int engine_ip_key_cmp(void *key1,
        void *key2)
{
    engine_ip_t *ipKey1 = (engine_ip_t*)key1;
    engine_ip_t *ipKey2 = (engine_ip_t*)key2;

    if (ipKey1->type != ipKey2->type)
    {
        return 1;
    }

    if (ipKey1->type == ADDRTYPE_IPV4)
    {
        if (ipKey1->ip_union._v4 == ipKey2->ip_union._v4)
        {
            return 0;
        }
    }
    else if (ipKey1->type == ADDRTYPE_IPV6)
    {
        if (IPV6_ADDR_EQUAL_COMMON(&ipKey1->ip_union._v6, &ipKey2->ip_union._v6))
        {
            return 0;
        }
    }

    return 1;
}

/*IPhash函数(网络字节序)*/
#define ENGINE_IP_KEY_HASH(key, mask) \
    engine_ip_t *ipKey = (engine_ip_t*)(key);\
    if (ipKey->type == 1)\
    {\
        return (ntohl(ipKey->ip_union._v4))&(mask);\
    }\
    else if (ipKey->type == 2)\
    {\
        return (ntohl(ipKey->ip_union._v6.s6_addr32[3])) & (mask);\
    }\
    return 0;


DLL_API bool host_addr_check(char *hostname);

DLL_API char* engine_ipv4_to_str(uint32_t ip, char *ip_buf);
DLL_API char* engine_ipv6_to_str(struct in6_addr *ipv6, char *ip_buf);
DLL_API char* engine_ip_to_str(engine_ip_t *ip, char *ip_buf);
DLL_API int engine_str_to_ipv4(const char* ipstr, uint32_t *ip);
DLL_API int engine_str_to_ipv6(const char* ipstr, struct in6_addr *ipv6);
DLL_API int engine_str_to_ip(const char* ipstr, engine_ip_t *ip);


#ifdef __cplusplus
}
#endif

#endif

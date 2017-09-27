#ifndef _ENGINE_IP_RANGE_H
#define _ENGINE_IP_RANGE_H

#include "commtype.h"
#include "engine_ip.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define SCAN_PORT_MIN 16
#define SCAN_PORT_MAX 65535


typedef struct{
    engine_ip_t start_addr;
    engine_ip_t end_addr;
}engine_ip_range_t;

typedef struct
{
    unsigned short start_port;
    unsigned short end_port;
}port_range_t;


#define  CONVERT_TO_BEGIN_END(minIP, maxIP, i_mask) \
        minIP &= (uint32_t)(((uint64_t)0xffffffff) << (32 - (i_mask))); \
        maxIP = (minIP) + (((uint64_t)0xffffffff) >> (i_mask));

DLL_API int engine_ip_parse_range(char* ipstr, engine_ip_range_t* ipRange);
DLL_API int engine_ip_parse_ranges(char* ipRangeStr,
                        engine_ip_range_t* ipRanges, 
                        int *ipRangesCnt,
                        int maxCnt);

DLL_API int port_parse_range(char* portstr, port_range_t* portRange);
DLL_API int port_parse_ranges(char* portRangeStr,
                        port_range_t* portRanges, 
                        int *portRangesCnt,
                        int maxCnt);

#ifdef __cplusplus
}
#endif

#endif

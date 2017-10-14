
#ifndef _SOCKS_MEM_H
#define _SOCKS_MEM_H


#include "commtype.h"
#include "logproc.h"

DLL_API void* socks_malloc();
DLL_API void socks_free(void* ptr);

DLL_API int socks_mem_init(uint32_t node_cnt);
DLL_API void socks_mem_destroy();

DLL_API unsigned int socks_mem_get_used_cnt();
DLL_API unsigned int socks_mem_get_free_cnt();

#endif

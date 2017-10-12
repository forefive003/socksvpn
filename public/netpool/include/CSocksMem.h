
#ifndef _SOCKS_MEM_H
#define _SOCKS_MEM_H


extern void* socks_malloc();
extern void socks_free(void* ptr);

extern int socks_mem_init(uint32_t node_cnt);
extern void socks_mem_destroy();

extern unsigned int socks_mem_get_used_cnt();

#endif

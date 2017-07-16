#include "commtype.h"
#include "logproc.h"
#include "mempool.h"
#include "CSocksMem.h"
#include "CNetRecv.h"

static pthread_spinlock_t g_socks_mem_lock;
static MEM_POOL_T g_socks_mem_pool;

void* socks_malloc()
{
	void *ptr = NULL;

	pthread_spin_lock(&g_socks_mem_lock);
	ptr = mpool_malloc(&g_socks_mem_pool);
	pthread_spin_unlock(&g_socks_mem_lock);
	return ptr;
}

void socks_free(void* ptr)
{
	pthread_spin_lock(&g_socks_mem_lock);
	mpool_free(&g_socks_mem_pool, ptr);
	pthread_spin_unlock(&g_socks_mem_lock);
}

int socks_mem_init(uint32_t node_cnt)
{
	pthread_spin_init(&g_socks_mem_lock, 0);
	if(mpool_init(&g_socks_mem_pool, node_cnt, sizeof(buf_node_t)) == -1)
	{
		_LOG_ERROR("mpool init failed");
		return -1;
	}

	return 0;
}

void socks_mem_destroy()
{
	mpool_destroy(&g_socks_mem_pool);
	pthread_spin_destroy(&g_socks_mem_lock);
}

unsigned int socks_mem_get_used_cnt()
{
	return mpoll_get_used_cnt(&g_socks_mem_pool);
}

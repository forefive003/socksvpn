#include "commtype.h"
#include "logproc.h"
#include "mempool.h"
#include "CSocksMem.h"
#include "CNetRecv.h"

#ifdef _WIN32
static LONG g_socks_mem_lock;
#else
static pthread_spinlock_t g_socks_mem_lock;
#endif

static MEM_POOL_T g_socks_mem_pool;

void* socks_malloc()
{
	void *ptr = NULL;

#ifdef _WIN32
	while (InterlockedExchange(&g_socks_mem_lock, 1) == 1){
		sleep_s(0);
	}
#else
	pthread_spin_lock(&g_socks_mem_lock);
#endif

	ptr = mpool_malloc(&g_socks_mem_pool);

#ifdef _WIN32
	InterlockedExchange(&g_socks_mem_lock, 0);
#else
	pthread_spin_unlock(&g_socks_mem_lock);
#endif
	return ptr;
}

void socks_free(void* ptr)
{
#ifdef _WIN32
	while (InterlockedExchange(&g_socks_mem_lock, 1) == 1){
		sleep_s(0);
	}
#else
	pthread_spin_lock(&g_socks_mem_lock);
#endif

	mpool_free(&g_socks_mem_pool, ptr);
	
#ifdef _WIN32
	InterlockedExchange(&g_socks_mem_lock, 0);
#else
	pthread_spin_unlock(&g_socks_mem_lock);
#endif
}

int socks_mem_init(uint32_t node_cnt)
{
#ifdef _WIN32
	g_socks_mem_lock = 0;
#else
    pthread_spin_init(&g_socks_mem_lock, 0);
#endif

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

#ifdef _WIN32
	g_socks_mem_lock = 0;
#else
    pthread_spin_destroy(&g_socks_mem_lock);
#endif
}

unsigned int socks_mem_get_used_cnt()
{
	return mpoll_get_used_cnt(&g_socks_mem_pool);
}


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"
#include "mempool.h"


int mpool_init(MEM_POOL_T *pool,
		unsigned int obj_max_cnt,
		unsigned int node_size)
{
	pool->m_node_size = node_size + sizeof(MEM_NODE_T);
	pool->m_node_cnt = obj_max_cnt;
	pool->m_baseaddr = (char*)malloc(pool->m_node_cnt * pool->m_node_size);
	if (NULL == pool->m_baseaddr)
	{
		printf("malloc error\n");
		return -1;
	}
	memset(pool->m_baseaddr, 0, pool->m_node_cnt * pool->m_node_size);

	pool->m_stack = (MEM_NODE_T**)malloc(pool->m_node_cnt * sizeof(MEM_NODE_T*));
	if (NULL == pool->m_stack)
	{
		printf("malloc error\n");
		return -1;
	}

	pool->m_stack_idx = 0;
	for (unsigned int i = 0; i < pool->m_node_cnt; i++)
	{
		mpool_free_obj(pool, (MEM_NODE_T*)&pool->m_baseaddr[i*pool->m_node_size]);
	}

	return 0;
}

void mpool_destroy(MEM_POOL_T *pool)
{
	free(pool->m_baseaddr);
	pool->m_baseaddr = NULL;
	free(pool->m_stack);
	pool->m_stack = NULL;
	pool->m_stack_idx = 0;
}

void mpool_reset(MEM_POOL_T *pool)
{
	pool->m_stack_idx = 0;
	for (unsigned int i = 0; i < pool->m_node_cnt; i++)
	{
		mpool_free_obj(pool, (MEM_NODE_T*)&pool->m_baseaddr[i*pool->m_node_size]);
	}
}

MEM_NODE_T* mpool_new_obj(MEM_POOL_T *pool)
{
	if (pool->m_stack_idx == 0)
	{
		return NULL;
	}

	return pool->m_stack[pool->m_stack_idx--];
}

void mpool_free_obj(MEM_POOL_T *pool, MEM_NODE_T *node)
{
	if (pool->m_stack_idx >= pool->m_node_cnt)
	{
		return;
	}

	INIT_LIST_HEAD(&node->node);
	pool->m_stack[pool->m_stack_idx] = node;
	pool->m_stack_idx++;	
}

void* mpool_malloc(MEM_POOL_T *pool)
{
	if (pool->m_stack_idx == 0)
	{
		return NULL;
	}

	pool->m_stack_idx--;
	return pool->m_stack[pool->m_stack_idx]->data;
}

void mpool_free(MEM_POOL_T *pool, void *ptr)
{
	if (pool->m_stack_idx >= pool->m_node_cnt)
	{
		return;
	}

	MEM_NODE_T *node = (MEM_NODE_T*)((char*)ptr - sizeof(struct list_head));
	INIT_LIST_HEAD(&node->node);
	pool->m_stack[pool->m_stack_idx] = node;
	pool->m_stack_idx++;
}

unsigned int mpoll_get_used_cnt(MEM_POOL_T *pool)
{
	return pool->m_node_cnt - pool->m_stack_idx;
}

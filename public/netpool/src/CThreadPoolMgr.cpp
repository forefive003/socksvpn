#include <stdio.h>
#include <signal.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <pthread.h>
#endif

#include "netpool.h"
#include "CThreadPool.h"
#include "CThreadPoolMgr.h"
#include "CJobTime.h"

CThreadPoolMgr *g_ThreadPoolMgr = NULL;

CThreadPoolMgr::CThreadPoolMgr(void)
{
	m_worker_thrd_pool = NULL;
	m_worker_thrd_cnt = 0;
}

CThreadPoolMgr::~CThreadPoolMgr(void)
{

}

BOOL CThreadPoolMgr::init_worker_thrds(unsigned int max_thrd_cnt,
		unsigned int start_core, unsigned int core_cnt)
{
	if (0 == max_thrd_cnt)
	{
		/*不需要额外线程*/
		//_LOG_ERROR("thrd cnt %d invalid.", max_thrd_cnt);
		return false;
	}

	m_worker_thrd_pool = new CThreadPool();
	if (NULL == m_worker_thrd_pool)
	{
		_LOG_ERROR("no memory when malloc.");
		return false;
	}

	if (false == m_worker_thrd_pool->Init(1,max_thrd_cnt,
						"workerThrds", start_core, core_cnt))
	{
		_LOG_ERROR("init worker thread pool failed.");
		return false;
	}

	m_worker_thrd_cnt = max_thrd_cnt;
	return true;
}

void CThreadPoolMgr::set_worker_thrds_func(thrd_init_func init_func,
				thrd_beat_func beat_func,
				thrd_exit_func exit_func)
{
	m_worker_thrd_pool->set_debug_func(init_func, beat_func, exit_func);
}


void* CThreadPoolMgr::init_evt_thrds(unsigned int max_thrd_cnt,
					unsigned int start_core,
					unsigned int core_cnt)
{
	if (1 > max_thrd_cnt)
	{
		_LOG_ERROR("thrd cnt %d invalid.", max_thrd_cnt);
		return NULL;
	}

	CThreadPool *newPool = new CThreadPool();
	if (NULL == newPool)
	{
		_LOG_ERROR("no memory when malloc.");
		return NULL;
	}

	if (false == newPool->Init(1,max_thrd_cnt,
				"eventThrds", start_core, core_cnt))
	{
		_LOG_ERROR("init event thread pool failed.");
		delete newPool;
		return NULL;
	}

	m_evt_thrd_pools.insert(newPool);
	return (void*)newPool;
}

void CThreadPoolMgr::free_evt_thrds(void* thrdPool, BOOL quickFree)
{
	if (NULL != thrdPool)
	{
		((CThreadPool*)thrdPool)->Terminate(quickFree);

		m_evt_thrd_pools.erase(((CThreadPool*)thrdPool));
		delete ((CThreadPool*)thrdPool);
	}
}

unsigned int CThreadPoolMgr::evt_task_cnt(void *thrdPool)
{
	return ((CThreadPool*)thrdPool)->GetTaskSize();
}

void CThreadPoolMgr::set_evt_thrds_func(void* thrdPool, 
				thrd_init_func init_func,
				thrd_beat_func beat_func,
				thrd_exit_func exit_func)
{
	((CThreadPool*)thrdPool)->set_debug_func(init_func, beat_func, exit_func);
}


void CThreadPoolMgr::_poll_timer_callback(void *param1, void *param2, void* param3, void* param4)
{
	CThreadPoolMgr *poolMgr = (CThreadPoolMgr*)param1;

	time_t nowTime = time(NULL);
	unsigned long long agedTime = nowTime - THRD_AGE_TIME;

	CThreadPool* thrdPool = NULL;
	ThrdPoolSetIter itr = poolMgr->m_evt_thrd_pools.begin();
	while(itr != poolMgr->m_evt_thrd_pools.end())
	{
		thrdPool = *itr;
		itr++;

		thrdPool->print_info();
		thrdPool->aged_worker(agedTime);
	}
}

BOOL CThreadPoolMgr::start()
{
	g_TimeJobMgr->add_time_job(CThreadPoolMgr::_poll_timer_callback,
					(void*)this, NULL, NULL, NULL,
					30, false);
	return true;
}

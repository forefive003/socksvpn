#ifndef _THREAD_POOL_MGR_H
#define _THREAD_POOL_MGR_H

#define THRD_AGE_TIME 10

typedef std::set<CThreadPool *> ThrdPoolSet;
typedef ThrdPoolSet::iterator ThrdPoolSetIter;

class CThreadPoolMgr
{
public:
    CThreadPoolMgr(void);
    ~CThreadPoolMgr(void);
    static CThreadPoolMgr* instance()
    {
        static CThreadPoolMgr *threadPoolMgr = NULL;

        if(threadPoolMgr == NULL)
        {
            threadPoolMgr = new CThreadPoolMgr();
        }
        return threadPoolMgr;
    }

public:
    BOOL init_worker_thrds(unsigned int max_thrd_cnt,
			unsigned int start_core, unsigned int core_cnt);
    void set_worker_thrds_func(thrd_init_func init_func,
					thrd_beat_func beat_func,
					thrd_exit_func exit_func);

    void* init_evt_thrds(unsigned int max_thrd_cnt,
						unsigned int start_core,
						unsigned int core_cnt);
    void set_evt_thrds_func(void *thrdPool, thrd_init_func init_func,
					thrd_beat_func beat_func,
					thrd_exit_func exit_func);

	void free_evt_thrds(void* thrdPool, BOOL quickFree = false);
	unsigned int evt_task_cnt(void *thrdPool);
	
	BOOL start();

private:
	static void _poll_timer_callback(void *param1, void *param2, void* param3, void* param4);

public:
	ThrdPoolSet m_evt_thrd_pools;

	CThreadPool *m_worker_thrd_pool;
	uint32_t m_worker_thrd_cnt;
};

extern CThreadPoolMgr *g_ThreadPoolMgr;

#endif

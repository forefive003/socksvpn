
#ifndef __THREAD_POOL_EXECUTOR__
#define __THREAD_POOL_EXECUTOR__

#include <set>
#include <list>

#include "CThread.h"

enum
{
	WORKER_INIT = 0,
	WORKER_STARTED,
	WORKER_STOPPING,
	WORKER_STOPPED
};


class CThreadPool
{
public:
    CThreadPool(void);
    ~CThreadPool(void);

    thrd_init_func m_init_func;
    thrd_beat_func m_beat_func;
    thrd_exit_func m_exit_func;
    volatile BOOL m_bQuickFree; /*是否快速退出。快速退出时已经在队列中的任务不再执行*/

    BOOL Init(unsigned int minThreads, unsigned int maxThreads,
    		const char *name,
			unsigned int startCore,
			unsigned int coreCnt,
			thrd_init_func init_func = NULL,
			thrd_beat_func beat_func = NULL,
			thrd_exit_func exit_func = NULL);

    BOOL Execute(Runnable * pRunnable);
    void volatile Terminate(BOOL quickFree = false);

    unsigned int GetThreadPoolSize();
    unsigned int GetTaskSize();
    Runnable * GetOneTask();

    BOOL IsAllWorkerBusy();
    void TellAllWorkerExit();

    void set_debug_func(thrd_init_func init_func,
                        thrd_beat_func beat_func,
                        thrd_exit_func exit_func);
    
#ifndef _WIN32
    pthread_mutex_t m_job_lock;
	pthread_cond_t m_job_notify;
#else
	HANDLE m_Event;
#endif

    unsigned int get_new_core();
    void print_info();
    void aged_worker(unsigned long long agetime);

private:
    class CWorker : public CThread
    {
    public:
        CWorker(CThreadPool * pThreadPool);
        ~CWorker();
        void Run();
        void TellExit();

        BOOL Aged(unsigned long long agetime);

        volatile BOOL m_isBusy;
        unsigned long m_run_cnt;

    private:
        CThreadPool * m_pThreadPool;

        unsigned long long m_update_time;
        volatile int m_workerStatus;
    };

    typedef std::set<CWorker *> ThreadPool;
    typedef std::list<Runnable *> Tasks;
    typedef Tasks::iterator TasksItr;
    typedef ThreadPool::iterator ThreadPoolItr;

    ThreadPool m_ThreadPool;
    ThreadPool m_TrashThread;
    Tasks m_Tasks;

    MUTEX_TYPE m_csTasksLock;
    MUTEX_TYPE m_csThreadPoolLock;

    volatile BOOL m_bRun;
    volatile BOOL m_bEnableInsertTask;
    volatile unsigned int m_minThreads;
    volatile unsigned int m_maxThreads;
    volatile unsigned int m_maxPendingTasks;

    const char *m_name;

    unsigned int m_cur_core_id;
    unsigned int m_start_core_id;
    unsigned int m_use_core_cnt;
};

#endif


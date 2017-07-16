
#ifndef _JOB_TIME_H
#define _JOB_TIME_H

#include "CThread.h"
#include "CThreadPool.h"

class CEvtTask : public Runnable
{
public:
	CEvtTask(evt_hdl_func evt_func,	void* param1,void* param2, void* param3, void* param4)
	{
		m_evt_func = evt_func;
		m_param1 = param1;
		m_param2 = param2;
		m_param3 = param3;
		m_param4 = param4;
	}
	~CEvtTask(void)
	{

	}

	void Run()
	{
		m_evt_func(m_param1, m_param2, m_param3, m_param4);
	}

private:
	evt_hdl_func m_evt_func;
	void* m_param1;
	void* m_param2;
	void* m_param3;
	void* m_param4;
};

class CTimeJob
{
public:
	CTimeJob(expire_hdl_func expire_func,
					void* param1, void* param2, void* param3, void* param4,
					unsigned int time_value,
					BOOL isOnce);
	virtual ~CTimeJob();
	
	void doit();
	BOOL is_expired(unsigned long long nowtime);
	void update_expired_time(unsigned long long nowtime);

	BOOL is_self(expire_hdl_func expire_func, void* param1);
	BOOL is_once();
private:
	expire_hdl_func  m_job_func;
	void *m_param1;
	void *m_param2;
	void *m_param3;
	void *m_param4;

	unsigned long long m_time_value;
	BOOL m_is_once;

	unsigned long long m_expire_time;
};

typedef std::list<CTimeJob*> TIME_LIST;
typedef TIME_LIST::iterator TIME_LIST_Itr;


class CTimeJobMgr
{
	friend class CTimeJob;
public:
	CTimeJobMgr();
    virtual ~CTimeJobMgr();
	static CTimeJobMgr* instance()
    {
        static CTimeJobMgr *timeJobMgr = NULL;

        if(timeJobMgr == NULL)
        {
            timeJobMgr = new CTimeJobMgr();
        }
        return timeJobMgr;
    }

public:
	void begin(unsigned int thrd_index);
	void run(unsigned int thrd_index);

	BOOL add_time_job(expire_hdl_func expire_func,
					void* param1, void* param2, void* param3, void* param4,
					unsigned int time_value,
					BOOL isOnce);
	BOOL del_time_job(expire_hdl_func expire_func, void* param1);

private:
	void add_time_node(CTimeJob *timeNode);
	void del_time_node(CTimeJob *timeNode);

	void time_job_walk(unsigned long long nowtime);
	CTimeJob* find_time_jobs(expire_hdl_func expire_func, void* param1);

private:
	MUTEX_TYPE m_job_lock;
	TIME_LIST m_time_jobs;

	unsigned long long m_latest_expire_time;
};

extern CTimeJobMgr *g_TimeJobMgr;

#endif

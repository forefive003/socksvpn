#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "netpool.h"
#include "CJobTime.h"
#include "CThreadPoolMgr.h"

CTimeJob::CTimeJob(expire_hdl_func expire_func,
					void* param1, void* param2, void* param3, void* param4,
					unsigned int time_value,
					BOOL isOnce)
{
	m_job_func = expire_func;
	m_param1 = param1;
	m_param2 = param2;
	m_param3 = param3;
	m_param4 = param4;

	m_time_value = time_value;
	m_is_once = isOnce;

	m_expire_time = util_get_cur_time() + time_value;
}

CTimeJob::~CTimeJob()
{

}

BOOL CTimeJob::is_expired(unsigned long long nowtime)
{
	return (m_expire_time <= nowtime);
}

void CTimeJob::update_expired_time(unsigned long long nowtime)
{
	this->m_expire_time = nowtime + this->m_time_value;
}

BOOL CTimeJob::is_self(expire_hdl_func expire_func, void* param1)
{
	return ((m_job_func == expire_func) && (param1 == m_param1));
}

BOOL CTimeJob::is_once()
{
	return m_is_once;
}

void CTimeJob::doit()
{
	m_job_func(m_param1, m_param2, m_param3, m_param4);
}


CTimeJobMgr *g_TimeJobMgr = NULL;

CTimeJobMgr::CTimeJobMgr()
{
	MUTEX_SETUP(m_job_lock);
}

CTimeJobMgr::~CTimeJobMgr()
{
	MUTEX_CLEANUP(m_job_lock);
}

void CTimeJobMgr::begin(unsigned int thrd_index)
{
	if (thrd_index != 0)
		return;

	this->m_latest_expire_time = util_get_cur_time();
}

void CTimeJobMgr::run(unsigned int thrd_index)
{
	if (thrd_index != 0)
		return;
	
	uint64_t nowtime = util_get_cur_time();
	unsigned int eclapse_time = nowtime - this->m_latest_expire_time;
	if (eclapse_time >= 1)
	{
		g_TimeJobMgr->time_job_walk(nowtime);
	}
}

void CTimeJobMgr::time_job_walk(unsigned long long nowtime)
{
	TIME_LIST_Itr itr;
    CTimeJob *pTimeJob = NULL;

	MUTEX_LOCK(m_job_lock);

    for (itr = m_time_jobs.begin();
            itr != m_time_jobs.end(); )
    {
        pTimeJob = *itr;

        /*maybe timenode deleted in callback func, after that, itr not valid*/
        itr++;
        
        if (pTimeJob->is_expired(nowtime))
        {
        	pTimeJob->update_expired_time(nowtime);
        	pTimeJob->doit();

			if (pTimeJob->is_once())
			{
				g_TimeJobMgr->del_time_node(pTimeJob);
				delete pTimeJob;
			}
        }
    }

	MUTEX_UNLOCK(m_job_lock);
}

void CTimeJobMgr::add_time_node(CTimeJob *timeNode)
{
    m_time_jobs.push_back(timeNode);
    _LOG_INFO("add timer");
}

void CTimeJobMgr::del_time_node(CTimeJob *timeNode)
{
    m_time_jobs.remove(timeNode);
    _LOG_INFO("del timer");
}

CTimeJob* CTimeJobMgr::find_time_jobs(expire_hdl_func expire_func, void* param1)
{
	TIME_LIST_Itr itr;
    CTimeJob *pTimeJob = NULL;

    for (itr = m_time_jobs.begin();
            itr != m_time_jobs.end();
            itr++)
    {
        pTimeJob = *itr;
        if (pTimeJob->is_self(expire_func, param1))
        {
            return pTimeJob;
        }
    }

    return NULL;
}

BOOL CTimeJobMgr::add_time_job(expire_hdl_func expire_func,
					void* param1, void* param2, void* param3, void* param4,
					unsigned int time_value,
					BOOL isOnce)
{
	CTimeJob *timeNode = new CTimeJob(expire_func, param1, param2, param3, param4, time_value, isOnce);
	assert(timeNode);
	this->add_time_node(timeNode);

	return true;
}

BOOL CTimeJobMgr::del_time_job(expire_hdl_func expire_func, void* param1)
{
	CTimeJob *pTimeJob = NULL;

	MUTEX_LOCK(m_job_lock);
	pTimeJob = find_time_jobs(expire_func, param1);
	if (NULL == pTimeJob)
	{
		MUTEX_UNLOCK(m_job_lock);
		_LOG_ERROR("node not exist when del time job.");
		return false;
	}
	
	del_time_node(pTimeJob);
	MUTEX_UNLOCK(m_job_lock);

	delete pTimeJob;
	return true;
}

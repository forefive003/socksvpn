
#include <stdio.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/time.h>
#endif

#include "netpool.h"
#include "CThread.h"


CThread::~CThread(void)
{
}
CThread::CThread():
		m_pRunnable(NULL),
		m_bRun(false)
{
}

CThread::CThread(Runnable * pRunnable):
		m_pRunnable(pRunnable),
		m_bRun(false)
{
}

BOOL CThread::Start(unsigned int core_id)
{
    if(m_bRun)
    {
        return true;
    }

#ifndef _WIN32
    if (0 == pthread_create(&m_handle, NULL, StaticThreadFunc,
        				(void *)this))
	{
		m_bRun = true;

		if (core_id != NO_SPEC_CORE)
		{
			cpu_set_t cpu_info;
			CPU_ZERO(&cpu_info);
			CPU_SET(core_id, &cpu_info);

			if (0 != pthread_setaffinity_np(m_handle, sizeof(cpu_set_t), &cpu_info))
			{
				_LOG_ERROR("Could not set thread %lu's CPU Affinity to core %d, continuing...",
						(unsigned long)m_handle, core_id);
			}
			else
			{
				_LOG_DEBUG("set thread %lu's CPU Affinity to core %d.",
						(unsigned long)m_handle, core_id);
			}
		}
	}
#else
	m_handle = (HANDLE)_beginthreadex(NULL, 0,
						StaticThreadFunc, this,
						0, &m_ThreadID);
	m_bRun = (NULL != m_handle);
#endif

    return m_bRun;
}

void CThread::Run()
{
    if(!m_bRun)
    {
        return;
    }

    if(NULL != m_pRunnable)
    {
        m_pRunnable->Run();
    }
}

void CThread::SetExitFlag()
{
	m_bRun = false;
}

/*timeout is Milliseconds(ºÁÃë)*/
void volatile CThread::Join(int timeout)
{
    //if(!m_bRun)
    //{
    //    return;
    //}
#ifndef _WIN32
    int wait_sec = 0;
    while(m_bRun)
    {
    	if(timeout > 0)
    	{
			if (wait_sec >= (timeout * 10))
			{
				_LOG_ERROR("wait thead exit failed, kill -9 directly.");
				pthread_kill(m_handle, SIGQUIT);
				break;
			}
    	}

    	usleep(100000);
		wait_sec++;
    }
#else
    if(timeout <= 0)
	{
		timeout = INFINITE;
	}

	::WaitForSingleObject(m_handle, timeout);
#endif
}

BOOL CThread::Terminate(unsigned long ExitCode)
{
    if(!m_bRun)
    {
        return false;
    }

#ifndef _WIN32
    pthread_kill(m_handle, SIGKILL);
#else
    if(::TerminateThread(m_handle, ExitCode))
    {
        ::CloseHandle(m_handle);
        return true;
    }
#endif

    return true;
}

unsigned long int CThread::GetThreadID()
{
#ifndef _WIN32
	return m_handle;
#else
    return m_ThreadID;
#endif
}

#ifndef _WIN32
static void thrd_quit_signal(int signo)
{
	_LOG_DEBUG("receive signo %d.", signo);
	pthread_exit(0);
}

void* CThread::StaticThreadFunc(void * arg)
{
    CThread * pThread = (CThread *)arg;

    struct sigaction sa;
	memset(&sa,0,sizeof(sa));

	sa.sa_handler = thrd_quit_signal;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGQUIT);

	if (sigaction(SIGQUIT, &sa, NULL) == -1)
	{
		_LOG_ERROR("signal error!");
		pthread_exit(0);
	}

    pthread_detach(pthread_self());

    pThread->Run();
    pThread->SetExitFlag();

    return 0;
}
#else
unsigned int WINAPI CThread::StaticThreadFunc(void * arg)
{
    CThread * pThread = (CThread *)arg;

    pThread->Run();
    pThread->SetExitFlag();

    return 0;
}

#endif



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
#include "CJobIo.h"
#include "CJobIoMgr.h"

CIoJobMgr *g_IoJobMgr = NULL;

CIoJobMgr::CIoJobMgr() 
{
#ifndef _WIN32
    pthread_mutexattr_t mux_attr;
    memset(&mux_attr, 0, sizeof(mux_attr));
    pthread_mutexattr_settype(&mux_attr, PTHREAD_MUTEX_RECURSIVE);
    MUTEX_SETUP_ATTR(m_job_lock, &mux_attr);
#else
    MUTEX_SETUP_ATTR(m_job_lock, NULL);
#endif
}

CIoJobMgr::~CIoJobMgr()
{
    MUTEX_CLEANUP(m_job_lock);
};

CIoJob* CIoJobMgr::find_io_job(int fd)
{
    IOJOB_LIST_Itr itr;
    CIoJob *pIoJob = NULL;

    for (itr = m_io_jobs.begin();
            itr != m_io_jobs.end();
            itr++)
    {
        pIoJob = *itr;
        if (pIoJob->get_fd() == fd)
        {
            return pIoJob;
        }
    }

    return NULL;
}

void CIoJobMgr::add_io_job(CIoJob* ioJob)
{
	MUTEX_LOCK(m_job_lock);
    m_io_jobs.push_back(ioJob);
    MUTEX_UNLOCK(m_job_lock);
}

void CIoJobMgr::del_io_job(CIoJob* ioJob)
{
	MUTEX_LOCK(m_job_lock);
    m_io_jobs.remove(ioJob);
    MUTEX_UNLOCK(m_job_lock);
}

void CIoJobMgr::lock()
{
	MUTEX_LOCK(m_job_lock);
}

void CIoJobMgr::unlock()
{
	MUTEX_UNLOCK(m_job_lock);
}


int CIoJobMgr::walk_to_set_sets(fd_set &rset, fd_set &wset)
{
    IOJOB_LIST_Itr itr;
    CIoJob *pIoJob = NULL;
    int maxFd = 0;

    MUTEX_LOCK(m_job_lock);

    for (itr = m_io_jobs.begin();
            itr != m_io_jobs.end(); )
    {
        pIoJob = *itr;

        /*maybe timenode deleted in callback func, after that, itr not valid*/
        itr++;
        
        int fd = pIoJob->get_fd();
        if (fd > maxFd)
        {
            maxFd = fd;
        }

        if (pIoJob->io_event_read())
        {
            FD_SET(fd, &rset);
        }
        else if (pIoJob->io_event_write())
        {
            FD_SET(fd, &wset);
        }
        else
        {
            _LOG_ERROR("fd event invalid when set.");
        }
    }

    MUTEX_UNLOCK(m_job_lock);

    return maxFd;
}

void CIoJobMgr::walk_to_handle_sets(fd_set &rset, fd_set &wset)
{
    IOJOB_LIST_Itr itr;
    CIoJob *pIoJob = NULL;

    MUTEX_LOCK(m_job_lock);

    for (itr = m_io_jobs.begin();
            itr != m_io_jobs.end(); )
    {
        pIoJob = *itr;

        /*maybe timenode deleted in callback func, after that, itr not valid*/
        itr++;
        
        int fd = pIoJob->get_fd();

        if (pIoJob->io_event_read())
        {
            if(FD_ISSET(fd, &rset))
            {
                pIoJob->read_evt_handle();
            }
        }
        else if (pIoJob->io_event_write())
        {
            if(FD_ISSET(fd, &wset))
            {
                pIoJob->write_evt_handle();
            }
        }
        else
        {
            _LOG_ERROR("fd event invalid when handle.");
        }
    }

    MUTEX_UNLOCK(m_job_lock);
}


void CIoJobMgr::move_to_deling_job(CIoJob* ioJob)
{
    MUTEX_LOCK(m_job_lock);
    m_del_io_jobs.push_back(ioJob);
    MUTEX_UNLOCK(m_job_lock);
}

void CIoJobMgr::handle_deling_job(unsigned int thrd_index)
{
    IOJOB_LIST_Itr itr;
    IOJOB_LIST deleted_job;

    /*avoid to call free_callback in lock*/
    MUTEX_LOCK(m_job_lock);
    for (itr = m_del_io_jobs.begin();
            itr != m_del_io_jobs.end(); )
    {
        if ((*itr)->get_thrd_index() == thrd_index)
        {
            /*add to deleted list*/            
            deleted_job.push_back(*itr);
            itr = m_del_io_jobs.erase(itr);
        }
        else
        {
            itr++;
        }
    }
    MUTEX_UNLOCK(m_job_lock);

    /*really free*/
    for (itr = deleted_job.begin();
            itr != deleted_job.end(); )
    {
        /*free resource*/            
        (*itr)->free_callback();
        delete (*itr);

        itr = deleted_job.erase(itr);
    }
    
    return;
}

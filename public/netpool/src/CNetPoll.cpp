
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
#include "CThread.h"
#include "CThreadPool.h"
#include "CJobIo.h"
#include "CJobIoMgr.h"
#include "CJobTime.h"
#include "CThreadPoolMgr.h"
#include "CNetPoll.h"

CNetPoll *g_NetPoll = NULL;

CNetPoll::CNetPoll() {
#ifndef _WIN32
	MUTEX_SETUP_ATTR(m_ep_lock, NULL);
	for (int i = 0; i < 32; i ++)
	{
		m_epfd[i] = -1;
	}
#endif
	m_cur_thrd_index = 0;
	m_isShutDown = false;

	m_cur_worker_thrds = 0;
	MUTEX_SETUP(m_lock);

	m_init_func = NULL;
	m_beat_func = NULL;
	m_exit_func = NULL;
}

CNetPoll::~CNetPoll() {
	MUTEX_CLEANUP(m_lock);
}

void CNetPoll::set_debug_func(thrd_init_func init_func,
						thrd_beat_func beat_func,
						thrd_exit_func exit_func)
{
	m_init_func = init_func;
	m_beat_func = beat_func;
	m_exit_func = exit_func;
}

#ifndef _WIN32
void CNetPoll::loop_handle(void *arg, void *param2, void *param3, void *param4)
{
	CNetPoll *pollObj = (CNetPoll*)arg;
	int evt_fd_index = (int)(long)param2;

#define EVENTMAX 1024
	struct epoll_event events[EVENTMAX];
	int fd_num = 0;
	int i = 0;

#if 0 /*why to black signal*/
	sigset_t block_set;
	sigemptyset(&block_set);
	sigfillset(&block_set);
	sigdelset(&block_set,SIGSEGV);
	sigdelset(&block_set,SIGBUS);
	sigdelset(&block_set,SIGPIPE);

	if( pthread_sigmask(SIG_BLOCK, &block_set,NULL) != 0 )
	{
		char err_buf[64] = {0};
		_LOG_ERROR("pthread_sigmask error %s\n", str_error_s(err_buf, sizeof(err_buf), errno));
		pthread_exit((void*)-1);
	}
#endif


	if (pollObj->m_init_func != NULL)
	{
		pollObj->m_init_func();
	}

	_LOG_INFO("work thread[%u] started.", evt_fd_index);

	MUTEX_LOCK(pollObj->m_lock);
	pollObj->m_cur_worker_thrds++;
	MUTEX_UNLOCK(pollObj->m_lock);

	g_TimeJobMgr->begin(evt_fd_index);

	while(false == pollObj->m_isShutDown)
	{
		bzero(events, sizeof(events));

		if (pollObj->m_beat_func != NULL)
		{
			pollObj->m_beat_func();
		}

		g_IoJobMgr->handle_deling_job(evt_fd_index);

		//MUTEX_LOCK(pollObj->m_ep_lock);
		fd_num = epoll_wait(pollObj->m_epfd[evt_fd_index], events, EVENTMAX, EPOLL_TIMEOUT*1000);
		//MUTEX_UNLOCK(pollObj->m_ep_lock);
		if (fd_num < 0)
		{
			char err_buf[64] = {0};

			if (EINTR == errno || errno == EAGAIN)
			{
				_LOG_DEBUG("select returned, continue, %s.",
						str_error_s(err_buf, sizeof(err_buf), errno));
				continue;
			}

			_LOG_INFO("epoll_wait failed, %s.",
									str_error_s(err_buf, sizeof(err_buf), errno));
			break;
		}
		else if (fd_num > 0)
		{
			for (i = 0; i < fd_num; i++)
			{
				if (events[i].events & EPOLLIN)
				{
					CIoJob *job_node = (CIoJob*)events[i].data.ptr;
					if (NULL == job_node)
					{
						_LOG_ERROR("EPOLLIN happen but no job node.");
					}
					else
					{
						job_node->read_evt_handle();
					}
				}
				/*有可能既有读事件又有写事件*/
				if (events[i].events & EPOLLOUT)
				{
					/*find write fd and write, del EPOLLOUT flag, otherwise loop*/
					CIoJob *job_node = (CWrIoJob*)events[i].data.ptr;
					if (NULL == job_node)
					{
						_LOG_ERROR("EPOLLOUT happen but no job node.");
					}
					else
					{
						job_node->write_evt_handle();
					}
				}
				if (events[i].events & EPOLLHUP)
				{
					/*app will feel it, let app to close*/
					CIoJob *job_node = (CIoJob*)events[i].data.ptr;
					if (NULL == job_node)
					{
						_LOG_ERROR("EPOLLHUP happen but no job node.");
					}
					else
					{
						_LOG_WARN("fd %d get EPOLLHUP.", job_node->get_fd());
					}
				}
				//else
				//{
				//	_LOG_ERROR("epoll_wait unexpect event 0x%x.", events[i].events);
				//}
			}
		}

		/*do timeout jobs*/
		g_TimeJobMgr->run(evt_fd_index);
	}

	g_IoJobMgr->handle_deling_job(evt_fd_index);

	if (pollObj->m_exit_func != NULL)
	{
		pollObj->m_exit_func();
	}

	MUTEX_LOCK(pollObj->m_lock);
	pollObj->m_cur_worker_thrds--;
	MUTEX_UNLOCK(pollObj->m_lock);

	_LOG_INFO("work thread[%u] exited.", evt_fd_index);
}
#else
void CNetPoll::loop_handle(void *arg, void *param2, void *param3, void *param4)
{
	CNetPoll *pollObj = (CNetPoll*)arg;
	int evt_fd_index = (int)(long)param2;
	///TODO
	int fd_num = 0;
	int i = 0;

	if (pollObj->m_init_func != NULL)
	{
		pollObj->m_init_func();
	}

	_LOG_INFO("work thread[%u] started.", evt_fd_index);

	MUTEX_LOCK(pollObj->m_lock);
	pollObj->m_cur_worker_thrds++;
	MUTEX_UNLOCK(pollObj->m_lock);

	g_TimeJobMgr->begin(evt_fd_index);

	SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	while(false == pollObj->m_isShutDown)
	{
		fd_set rset;
		fd_set wset;
		FD_ZERO(&rset);
		FD_ZERO(&wset);

		if (pollObj->m_beat_func != NULL)
		{
			pollObj->m_beat_func();
		}

		g_IoJobMgr->handle_deling_job(evt_fd_index);

		int maxFd = 0;
		maxFd = g_IoJobMgr->walk_to_set_sets(rset, wset);
		//windows select must has one fd
		if (maxFd == 0)
		{
			FD_SET(hSocket, &rset);
			maxFd = hSocket;
		}

		struct timeval tv;
		tv.tv_sec = EPOLL_TIMEOUT;
		tv.tv_usec = 0;
		fd_num = select(maxFd + 1, &rset, &wset, NULL, &tv);
		if (fd_num < 0)
		{
			DWORD dwError = WSAGetLastError();
			if (EINTR == dwError || dwError == EAGAIN)
			{
				_LOG_INFO("select returned, continue, %d.", dwError);
				continue;
			}

			_LOG_INFO("epoll_wait failed, %d.", dwError);
			break;
		}
		else if (fd_num > 0)
		{
			g_IoJobMgr->walk_to_handle_sets(rset, wset);
		}

		/*do timeout jobs*/
		g_TimeJobMgr->run(evt_fd_index);
	}

	closesocket(hSocket);

	g_IoJobMgr->handle_deling_job(evt_fd_index);

	if (pollObj->m_exit_func != NULL)
	{
		pollObj->m_exit_func();
	}

	MUTEX_LOCK(pollObj->m_lock);
	pollObj->m_cur_worker_thrds--;
	MUTEX_UNLOCK(pollObj->m_lock);

	_LOG_INFO("work thread[%u exited.", evt_fd_index);
}
#endif

void CNetPoll::del_io_writing_evt(CIoJob *jobNode)
{
#ifndef _WIN32
	char err_buf[64] = {0};
	if (jobNode->io_event_read())
	{
		/*modify*/
		struct epoll_event ev;

		memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN | EPOLLET;
		ev.data.ptr = (void*)jobNode;
		if(epoll_ctl(m_epfd[jobNode->get_thrd_index()], EPOLL_CTL_MOD, jobNode->get_fd(), &ev) != 0)
		{
			_LOG_ERROR("EPOLL_CTL_MOD failed, %s.", str_error_s(err_buf, sizeof(err_buf), errno));
		}
		else
		{
			_LOG_DEBUG("pause write event from io job, fd %d.", jobNode->get_fd());
		}
	}
	else
	{
		/*del*/
		if(epoll_ctl(m_epfd[jobNode->get_thrd_index()], EPOLL_CTL_DEL, jobNode->get_fd(), NULL) != 0)
		{
			_LOG_ERROR("EPOLL_CTL_DEL failed, %s.", str_error_s(err_buf, sizeof(err_buf), errno));
		}
	}
#endif
}

BOOL CNetPoll::add_listen_job(accept_hdl_func io_func, int  fd, void* param1)
{
	char err_buf[64] = {0};

	g_IoJobMgr->lock();
	if (NULL != g_IoJobMgr->find_io_job(fd))
	{
		g_IoJobMgr->unlock();
		_LOG_ERROR("fd %d already has a listen job.", fd);
		return false;
	}

	CListenJob *job_node = new CListenJob(fd, param1);
	assert(job_node);
	job_node->set_read_callback((void*)io_func);
	job_node->add_read_io_event();
	job_node->set_thrd_index(0);

#ifndef _WIN32
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = (void*)job_node;
	//for (unsigned int i = 0; i < g_ThreadPoolMgr->m_worker_thrd_cnt; i++)
	{
		if(epoll_ctl(m_epfd[0], EPOLL_CTL_ADD, fd, &ev) != 0)
		{
			g_IoJobMgr->unlock();

			delete job_node;
			_LOG_ERROR("epoll_ctl add listen failed, fd %d, %s.", fd,
					str_error_s(err_buf, sizeof(err_buf), errno));
			assert(0);
			return false;
		}
	}	
#endif

	g_IoJobMgr->add_io_job(job_node);
	g_IoJobMgr->unlock();

	_LOG_INFO("add new listen job, fd %d.", fd);
	return true;
}

BOOL CNetPoll::del_listen_job(int fd, free_hdl_func free_func)
{
	char err_buf[64] = {0};

	CIoJob *job_node = NULL;
	BOOL ret = true;

	g_IoJobMgr->lock();
	job_node = g_IoJobMgr->find_io_job(fd);
	if (NULL == job_node)
	{
		g_IoJobMgr->unlock();
		_LOG_ERROR("listen job fd %d not exist.", fd);
		return false;
	}
	job_node->set_free_callback((void*)free_func);

	g_IoJobMgr->del_io_job(job_node);
	g_IoJobMgr->move_to_deling_job(job_node);

	g_IoJobMgr->unlock();

#ifndef _WIN32
	/*delete from epoll fd*/
	//for (unsigned int i = 0; i < g_ThreadPoolMgr->m_worker_thrd_cnt; i++)
	{
		if(epoll_ctl(m_epfd[0], EPOLL_CTL_DEL, fd, NULL) != 0)
		{
			_LOG_ERROR("EPOLL_CTL_DEL failed, %s.", str_error_s(err_buf, sizeof(err_buf), errno));
			return false;
		}
	}
#endif

	_LOG_INFO("del listen job, fd %d.", fd);
	return ret;
}

BOOL CNetPoll::add_read_job(read_hdl_func io_func,
						int  fd, void* param1,
						unsigned int thrd_index,
						int bufferSize, BOOL isTcp)
{
	char err_buf[64] = {0};

	CIoJob *job_node = NULL;

	g_IoJobMgr->lock();
	job_node = g_IoJobMgr->find_io_job(fd);
	if (NULL == job_node)
	{
		if (isTcp)
		{
			job_node = new CTcpJob(fd, param1);
		}
		else
		{
			job_node = new CUdpJob(fd, param1);
		}

		assert(job_node);
		job_node->lock();
		job_node->set_read_callback((void*)io_func);
		job_node->add_read_io_event();
		job_node->init_recv_buf(bufferSize);
		if (thrd_index >= g_ThreadPoolMgr->m_worker_thrd_cnt)
		{
			thrd_index = get_next_thrd_index();
		}
		job_node->set_thrd_index(thrd_index);
#ifndef _WIN32
		struct epoll_event ev;
		memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN | EPOLLET;
		ev.data.ptr = (void*)job_node;
		if(epoll_ctl(m_epfd[thrd_index], EPOLL_CTL_ADD, fd, &ev) != 0)
		{
			job_node->unlock();
			g_IoJobMgr->unlock();

			delete job_node;
			_LOG_ERROR("epoll_ctl add read failed, fd %d, %s.", fd,
					 str_error_s(err_buf, sizeof(err_buf), errno));
			return false;
		}
#endif
		job_node->unlock();

		g_IoJobMgr->add_io_job(job_node);
		_LOG_INFO("add new read job, fd %d.", fd);
		g_IoJobMgr->unlock();
		
		return true;
	}

	job_node->lock();
	
#ifndef _WIN32
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.data.ptr = (void*)job_node;
	ev.events = EPOLLIN | EPOLLET;
	if (job_node->io_event_write())
	{
		ev.events |= EPOLLOUT;
		if(epoll_ctl(m_epfd[job_node->get_thrd_index()], EPOLL_CTL_MOD, fd, &ev) != 0)
		{
			job_node->unlock();
			g_IoJobMgr->unlock();
			_LOG_ERROR("epoll_ctl mod to read failed, fd %d, epfd %d, errno %d, %s.",
								fd,	m_epfd[job_node->get_thrd_index()],
								errno, str_error_s(err_buf, sizeof(err_buf), errno));
			return false;
		}
	}
	else
	{
		if(epoll_ctl(m_epfd[thrd_index], EPOLL_CTL_ADD, fd, &ev) != 0)
		{
			job_node->unlock();
			g_IoJobMgr->unlock();
			_LOG_ERROR("epoll_ctl add read failed, fd %d, %s.", fd,
					 str_error_s(err_buf, sizeof(err_buf), errno));
			return false;
		}	
	}
	
#endif
	
	job_node->set_read_callback((void*)io_func);
	job_node->add_read_io_event();
	job_node->init_recv_buf(bufferSize);
	_LOG_INFO("modify job, add read event, fd %d.", fd);

	job_node->unlock();	
	g_IoJobMgr->unlock();
	
	return true;
}

BOOL CNetPoll::del_read_job(int fd, free_hdl_func free_func)
{
	char err_buf[64] = {0};

	CIoJob *job_node = NULL;

	g_IoJobMgr->lock();

	job_node = g_IoJobMgr->find_io_job(fd);
	if (NULL == job_node)
	{
		g_IoJobMgr->unlock();
		_LOG_ERROR("read job fd %d not exist.", fd);
		return false;
	}

	job_node->set_free_callback((void*)free_func);

	job_node->lock();

	if (job_node->io_event_write())
	{
#ifndef _WIN32
		/*modify*/
		struct epoll_event ev;

		memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLOUT | EPOLLET;
		ev.data.ptr = (void*)job_node;
		if(epoll_ctl(m_epfd[job_node->get_thrd_index()], EPOLL_CTL_MOD, fd, &ev) != 0)
		{
			_LOG_ERROR("EPOLL_CTL_MOD failed, %s.", str_error_s(err_buf, sizeof(err_buf), errno));
		}
#endif
		_LOG_INFO("del read event and set to write, fd %d", fd);
	}
	else if (job_node->io_event_read())
	{
#ifndef _WIN32
		/*delete from epoll fd*/
		if(epoll_ctl(m_epfd[job_node->get_thrd_index()], EPOLL_CTL_DEL, fd, NULL) != 0)
		{
			_LOG_ERROR("EPOLL_CTL_DEL failed, %s.", str_error_s(err_buf, sizeof(err_buf), errno));
		}
#endif
		_LOG_INFO("del read event and del job, fd %d", fd);
		job_node->set_deleting_flag();
		g_IoJobMgr->del_io_job(job_node);
		g_IoJobMgr->move_to_deling_job(job_node);
	}
	else
	{
		/*有可能job上什么事件都没有*/
		job_node->set_deleting_flag();
		g_IoJobMgr->del_io_job(job_node);
		g_IoJobMgr->move_to_deling_job(job_node);

		_LOG_INFO("del read job, fd %d.", fd);
	}

	job_node->del_read_io_event();
	job_node->unlock();

	g_IoJobMgr->unlock();
	return true;
}

BOOL CNetPoll::add_write_job(write_hdl_func io_func,
						int  fd, void* param1, 
						unsigned int thrd_index,
						BOOL isTcp)
{
	char err_buf[64] = {0};

	CIoJob *job_node = NULL;

	g_IoJobMgr->lock();
	job_node = g_IoJobMgr->find_io_job(fd);
	if (NULL == job_node)
	{
		if (isTcp)
		{
			job_node = new CTcpJob(fd, param1);
		}
		else
		{
			job_node = new CUdpJob(fd, param1);
		}

		assert(job_node);
		job_node->lock();
		job_node->set_write_callback((void*)io_func);
		job_node->add_write_io_event();
		if (thrd_index >= g_ThreadPoolMgr->m_worker_thrd_cnt)
		{
			thrd_index = get_next_thrd_index();
		}
		job_node->set_thrd_index(thrd_index);

#ifndef _WIN32
		struct epoll_event ev;
		memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLOUT | EPOLLET;
		ev.data.ptr = (void*)job_node;
		if(epoll_ctl(m_epfd[thrd_index], EPOLL_CTL_ADD, fd, &ev) != 0)
		{
			job_node->unlock();
			g_IoJobMgr->unlock();

			delete job_node;
			_LOG_ERROR("epoll_ctl add write failed, fd %d, %s.", fd,
					str_error_s(err_buf, sizeof(err_buf), errno));
			return false;
		}
#endif
		job_node->unlock();

		g_IoJobMgr->add_io_job(job_node);

		_LOG_INFO("add new write job, fd %d.", fd);
		g_IoJobMgr->unlock();

		return true;
	}

	job_node->set_write_callback((void*)io_func);

	job_node->lock();

	if (false == job_node->io_event_write())
	{
		_LOG_DEBUG("modify job, add write event, fd %d.", fd);
		job_node->add_write_io_event();

#ifndef _WIN32		
		struct epoll_event ev;
		memset(&ev, 0, sizeof(ev));
		ev.data.ptr = (void*)job_node;
		ev.events = EPOLLOUT;
		if (job_node->io_event_read())
		{
			ev.events |= EPOLLIN | EPOLLET;
			if(epoll_ctl(m_epfd[job_node->get_thrd_index()], EPOLL_CTL_MOD, fd, &ev) != 0)
			{
				job_node->unlock();
				g_IoJobMgr->unlock();
				_LOG_ERROR("epoll_ctl mod to write failed, fd %d, %s.",
									fd,	str_error_s(err_buf, sizeof(err_buf), errno));
				return false;
			}
		}
		else
		{
			if(epoll_ctl(m_epfd[thrd_index], EPOLL_CTL_ADD, fd, &ev) != 0)
			{
				job_node->unlock();
				g_IoJobMgr->unlock();
				_LOG_ERROR("epoll_ctl add write failed, fd %d, %s.", fd,
						str_error_s(err_buf, sizeof(err_buf), errno));
				return false;
			}
		}
#endif
	}
	
	job_node->unlock();
	g_IoJobMgr->unlock();
	return true;
}


BOOL CNetPoll::del_write_job(int fd, free_hdl_func free_func)
{
	char err_buf[64] = {0};

	CIoJob *job_node = NULL;

	g_IoJobMgr->lock();

	job_node = g_IoJobMgr->find_io_job(fd);
	if (NULL == job_node)
	{
		/*对于write事件, 在处理时会自动停止,因此如果del_read_job会直接删除,
		程序再调用此接口时job已经不存在,属于正常流程*/
		//_LOG_DEBUG("write job fd %d not exist when del.", fd);
		g_IoJobMgr->unlock();
		return true;
	}
	job_node->set_free_callback((void*)free_func);

	job_node->lock();
	if (job_node->io_event_read())
	{
#ifndef _WIN32
		/*modify*/
		struct epoll_event ev;

		memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN | EPOLLET;
		ev.data.ptr = (void*)job_node;
		if(epoll_ctl(m_epfd[job_node->get_thrd_index()], EPOLL_CTL_MOD, fd, &ev) != 0)
		{
			_LOG_ERROR("EPOLL_CTL_MOD failed, epfd %d, %s.",
				m_epfd[job_node->get_thrd_index()],
				str_error_s(err_buf, sizeof(err_buf), errno));
		}
#endif
		_LOG_INFO("del write event and set to read, fd %d", fd);
	}
	else if (job_node->io_event_write())
	{
#ifndef _WIN32
		/*delete from epoll fd*/
		if(epoll_ctl(m_epfd[job_node->get_thrd_index()], EPOLL_CTL_DEL, fd, NULL) != 0)
		{
			_LOG_ERROR("EPOLL_CTL_DEL failed, epfd %d, %s.", 
				m_epfd[job_node->get_thrd_index()],
				str_error_s(err_buf, sizeof(err_buf), errno));
		}

#endif
		_LOG_INFO("del write event and del job, fd %d", fd);

		job_node->set_deleting_flag();
		g_IoJobMgr->del_io_job(job_node);
		g_IoJobMgr->move_to_deling_job(job_node);
	}
	else
	{
		job_node->set_deleting_flag();
		g_IoJobMgr->del_io_job(job_node);
		g_IoJobMgr->move_to_deling_job(job_node);

		_LOG_INFO("del write job, fd %d.", fd);
	}

	job_node->del_write_io_event();
	job_node->unlock();

	g_IoJobMgr->unlock();
	return true;
}


BOOL CNetPoll::init_event_fds()
{
#ifndef _WIN32
	if (g_ThreadPoolMgr->m_worker_thrd_cnt > 0)
	{
		for (unsigned int i = 0; i < g_ThreadPoolMgr->m_worker_thrd_cnt; i++)
		{
			m_epfd[i] = epoll_create(1);
			if(m_epfd[i] == -1)
			{
				_LOG_ERROR("epoll_create failed.");
				assert(0);
			}			
		}
	}
	else
	{
		m_epfd[0] = epoll_create(1);
		if(m_epfd[0] == -1)
		{
			_LOG_ERROR("epoll_create failed.");
			assert(0);
		}
	}
#endif
	return TRUE;
}
unsigned int CNetPoll::get_next_thrd_index()
{
	m_cur_thrd_index++;
	if (m_cur_thrd_index >= g_ThreadPoolMgr->m_worker_thrd_cnt)
	{
		m_cur_thrd_index = 0;
	}
	return m_cur_thrd_index;
}

BOOL CNetPoll::start()
{
	m_isShutDown = false;

	if (NULL != g_ThreadPoolMgr->m_worker_thrd_pool)
	{
		unsigned int i = 0;

		for (i = 0; i < g_ThreadPoolMgr->m_worker_thrd_cnt; i++)
		{
			CEvtTask *listenTask = new CEvtTask((evt_hdl_func)loop_handle,
					(void*)this, (void*)(long)i, NULL, NULL);
			g_ThreadPoolMgr->m_worker_thrd_pool->Execute(listenTask);
		}
	}
	else
	{
		loop_handle((void*)this, (void*)(long)0, NULL, NULL);
	}

	return true;
}

void CNetPoll::let_stop()
{
	/*set to shutdown all thread*/
	m_isShutDown = true;
	/*maybe call this function in signal func, which easy to cause dead lock*/
	//_LOG_INFO("let loop to exit.");
}

void CNetPoll::wait_stop()
{
	/*wait*/
	int loopCnt = 0;
	while(m_cur_worker_thrds != 0)
	{
		usleep(100);
		loopCnt++;
		if (0 == (loopCnt % 60000))
		{
			_LOG_WARN("wait all loop exit, loop %d, cur %d.",
					loopCnt, m_cur_worker_thrds);
		}
	}

	_LOG_INFO("all loop exit succ.");

#ifndef _WIN32
	for (int i = 0; i < 32; i ++)
	{
		if (m_epfd[i] != -1)
		{
			close(m_epfd[i]);
			m_epfd[i] = -1;
		}
		m_epfd[i] = -1;
	}
	MUTEX_CLEANUP(m_ep_lock);
#endif
}

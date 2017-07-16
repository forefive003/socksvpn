
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "netpool.h"
#include "CThread.h"
#include "CThreadPool.h"
#include "CJobIo.h"
#include "CJobIoMgr.h"
#include "CJobTime.h"
#include "CThreadPoolMgr.h"
#include "CNetPoll.h"


DLL_API BOOL np_init_worker_thrds(unsigned int max_thrd_cnt,
					unsigned int start_core,
					unsigned int core_cnt)
{
	BOOL ret = FALSE;
	
	ret = g_ThreadPoolMgr->init_worker_thrds(max_thrd_cnt, start_core, core_cnt);
	ret = g_NetPoll->init_event_fds();
	return ret;
}
DLL_API void np_set_worker_thrds_debug_func(thrd_init_func init_func,
					thrd_beat_func beat_func,
					thrd_exit_func exit_func)
{
	g_ThreadPoolMgr->set_worker_thrds_func(init_func, beat_func, exit_func);
}


DLL_API void* np_init_evt_thrds(unsigned int max_thrd_cnt,
				unsigned int start_core,
				unsigned int core_cnt)
{
	return g_ThreadPoolMgr->init_evt_thrds(max_thrd_cnt, start_core, core_cnt);
}
DLL_API void np_set_evt_thrds_debug_func(void *thrdPool, thrd_init_func init_func,
					thrd_beat_func beat_func,
					thrd_exit_func exit_func)
{
	g_ThreadPoolMgr->set_evt_thrds_func(thrdPool, init_func, beat_func, exit_func);
}

DLL_API void np_free_evt_thrds(void* thrdPool, BOOL quickFree)
{
	return g_ThreadPoolMgr->free_evt_thrds(thrdPool, quickFree);
}

DLL_API unsigned int np_evt_jobs_cnt(void* thrdPool)
{
	return g_ThreadPoolMgr->evt_task_cnt(thrdPool);
}

DLL_API BOOL np_add_listen_job(accept_hdl_func acpt_func, int fd, void* param1)
{
	return g_NetPoll->add_listen_job(acpt_func, fd, param1);
}

DLL_API BOOL np_del_listen_job(int  fd, free_hdl_func free_func)
{
	return g_NetPoll->del_listen_job(fd, free_func);
}


DLL_API BOOL np_add_read_job(read_hdl_func read_func,
					int fd, void* param1,
					unsigned int thrd_index,
					int bufferSize)
{
	return g_NetPoll->add_read_job(read_func, fd, param1, thrd_index, bufferSize, true);
}

DLL_API BOOL np_add_udp_read_job(read_hdl_func read_func,
					int fd, void* param1,
					unsigned int thrd_index,
					int bufferSize)
{
	return g_NetPoll->add_read_job(read_func, fd, param1, thrd_index, bufferSize, false);
}

DLL_API BOOL np_del_read_job(int  fd, free_hdl_func free_func)
{
	return g_NetPoll->del_read_job(fd, free_func);
}

DLL_API BOOL np_add_write_job(write_hdl_func write_func,
					int  fd, void* param1,
					unsigned int thrd_index)
{
	return g_NetPoll->add_write_job(write_func, fd, param1, thrd_index, true);
}
DLL_API BOOL np_add_udp_write_job(write_hdl_func write_func,
					int  fd, void* param1,
					int thrd_index)
{
	return g_NetPoll->add_write_job(write_func, fd, param1, thrd_index, false);
}

DLL_API BOOL np_del_write_job(int  fd, free_hdl_func free_func)
{
	return g_NetPoll->del_write_job(fd, free_func);
}

DLL_API BOOL np_add_evt_job(void *thrdPool,
		evt_hdl_func evt_func, void* param1, void* param2, void *param3, void* param4)
{
	/*throw to the thread pool*/
	CEvtTask *evtTask = new CEvtTask(evt_func, param1, param2, param3, param4);
	if (NULL == evtTask)
	{
		_LOG_ERROR("no memory when malloc evtTask.");
		return false;
	}
	return ((CThreadPool*)thrdPool)->Execute(evtTask);
}

DLL_API BOOL np_add_time_job(expire_hdl_func expire_func,
				void* param1, void* param2, void* param3,void* param4,
				unsigned int time_value,
				BOOL isOnce)
{
	return g_TimeJobMgr->add_time_job(expire_func,
								param1, param2, param3, param4,
								time_value, isOnce);
}

DLL_API BOOL np_del_time_job(expire_hdl_func expire_func, void* param1)
{
	return g_TimeJobMgr->del_time_job(expire_func, param1);
}

DLL_API BOOL np_init()
{
#ifdef _WIN32
	char err_buf[64] = {0};

	WSADATA  Ws;
	if (WSAStartup(MAKEWORD(2,2), &Ws) != 0 )
	{
		_LOG_ERROR("Init Windows Socket Failed, %s!",
					str_error_s(err_buf, sizeof(err_buf), errno));
		return false;
	}
#endif
	g_ThreadPoolMgr = CThreadPoolMgr::instance();
	g_TimeJobMgr = CTimeJobMgr::instance();
	g_IoJobMgr = CIoJobMgr::instance();
	g_NetPoll = CNetPoll::instance();

	return true;
}

DLL_API void np_free()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

DLL_API BOOL np_start()
{
    if(false == g_ThreadPoolMgr->start())
    {
        _LOG_ERROR("thread pool mgr start failed.");
        return false;
    }

    if (false == g_NetPoll->start())
    {
    	_LOG_ERROR("net poll start failed.");
    	return false;
    }
	
	return true;
}

DLL_API void np_let_stop()
{
	g_NetPoll->let_stop();
}

DLL_API void np_wait_stop()
{
	g_NetPoll->wait_stop();
}

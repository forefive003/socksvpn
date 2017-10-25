
#ifndef NP_MAIN_H_
#define NP_MAIN_H_

#include "commtype.h"
#include "utilcommon.h"
#include "logproc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define  MAX_FD_CNT 65536
#define  MAX_THRD_CNT 64
#define  INVALID_THRD_INDEX  65

typedef void (*accept_hdl_func)(int  fd, void* param1);
typedef void (*udp_read_hdl_func)(int  fd, void* param1,
		struct sockaddr *cliAddr,
		char *recvBuf, int recvLen);
typedef void (*read_hdl_func)(int  fd, void* param1,
		char *recvBuf, int recvLen);
typedef void (*write_hdl_func)(int  fd, void* param1);

typedef void (*free_hdl_func)(int  fd, void* param1);

typedef void (*evt_hdl_func)(void* param1, void* param2, void* param3, void* param4);
typedef void (*expire_hdl_func)(void* param1, void* param2, void* param3, void* param4);

typedef int (*thrd_init_func)();
typedef void (*thrd_exit_func)();
typedef void (*thrd_beat_func)();

DLL_API int np_get_thrd_fdcnt(int thrd_index);
DLL_API UTIL_TID np_get_thrd_tid(int thrd_index);

DLL_API BOOL np_init_worker_thrds(unsigned int max_thrd_cnt,
					unsigned int start_core = 0,
					unsigned int core_cnt = 0);
DLL_API void np_set_worker_thrds_debug_func(thrd_init_func init_func,
					thrd_beat_func beat_func,
					thrd_exit_func exit_func);

DLL_API void* np_init_evt_thrds(unsigned int max_thrd_cnt,
		unsigned int start_core,
		unsigned int core_cnt);
DLL_API void np_set_evt_thrds_debug_func(void *thrdPool, thrd_init_func init_func,
					thrd_beat_func beat_func,
					thrd_exit_func exit_func);

DLL_API void np_free_evt_thrds(void* thrdPool, BOOL quickFree = FALSE);
DLL_API unsigned int np_evt_jobs_cnt(void* thrdPool);

DLL_API BOOL np_add_listen_job(accept_hdl_func acpt_func, 
				int fd, void* param1, 
				int thrd_index = INVALID_THRD_INDEX);
DLL_API BOOL np_del_listen_job(int  fd, free_hdl_func free_func = NULL);

DLL_API BOOL np_set_job_free_callback(int fd, free_hdl_func free_func);

DLL_API BOOL np_add_read_job(read_hdl_func read_func,
				int  fd, void* param1,
				int thrd_index = INVALID_THRD_INDEX,
				int bufferSize = 0);
DLL_API BOOL np_add_udp_read_job(udp_read_hdl_func read_func,
				int  fd, void* param1,
				int thrd_index = INVALID_THRD_INDEX,
				int bufferSize = 0);
DLL_API BOOL np_del_read_job(int  fd, free_hdl_func free_func = NULL);


DLL_API BOOL np_pause_read_on_job(int  fd);
DLL_API BOOL np_resume_read_on_job(int  fd);
DLL_API BOOL np_del_io_job(int fd, free_hdl_func free_func = NULL);

DLL_API BOOL np_add_write_job(write_hdl_func write_func,
				int  fd, void* param1,
				int thrd_index = INVALID_THRD_INDEX);
DLL_API BOOL np_add_udp_write_job(write_hdl_func write_func,
				int  fd, void* param1);
DLL_API BOOL np_del_write_job(int  fd, free_hdl_func free_func = NULL);


DLL_API BOOL np_add_time_job(expire_hdl_func expire_func,
				void* param1, void* param2, void* param3, void* param4,
				unsigned int time_value,
				BOOL isOnce=false);
DLL_API BOOL np_del_time_job(expire_hdl_func expire_func, void* param1);


DLL_API BOOL np_add_evt_job(void *thrdPool,
		evt_hdl_func evt_func, void* param1, void* param2, void *param3, void* param4);

DLL_API BOOL np_init();
DLL_API void np_free();

DLL_API BOOL np_start();
DLL_API void np_let_stop();
DLL_API void np_wait_stop();

#ifdef __cplusplus
}
#endif


#endif /* EPT_H_ */

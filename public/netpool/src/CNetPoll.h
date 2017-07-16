
#ifndef CNETPOLL_H_
#define CNETPOLL_H_


#define EPOLL_TIMEOUT 1  /*1S*/

class CNetPoll {
public:
	CNetPoll();
	virtual ~CNetPoll();
	static CNetPoll* instance()
    {
        static CNetPoll *netPoll = NULL;

        if(netPoll == NULL)
        {
            netPoll = new CNetPoll();
        }
        return netPoll;
    }
public:
	void set_debug_func(thrd_init_func init_func,
						thrd_beat_func beat_func,
						thrd_exit_func exit_func);

	BOOL start();
	void let_stop();
	void wait_stop();

	BOOL add_listen_job(accept_hdl_func acpt_func,
								int fd, void* param1);
	BOOL del_listen_job(int fd, free_hdl_func free_func);

	BOOL add_read_job(read_hdl_func read_func,
					int fd, 
					void* param1,
					unsigned int thrd_index,
					int bufferSize,
					BOOL isTcp);
	BOOL del_read_job(int  fd, free_hdl_func free_func);

	BOOL add_write_job(write_hdl_func io_func,
						int  fd, 
						void* param1, 
						unsigned int thrd_index,
						BOOL isTcp);
	BOOL del_write_job(int  fd, free_hdl_func free_func);
	void del_io_writing_evt(CIoJob *jobNode);

	BOOL init_event_fds();

private:
	static void loop_handle(void *arg, void *param2, void *param3, void *param4);
	unsigned int get_next_thrd_index();

private:
	unsigned int m_cur_thrd_index;

#ifndef _WIN32
	MUTEX_TYPE m_ep_lock;
	int  m_epfd[32];
#endif
	uint32_t m_cur_worker_thrds;
	MUTEX_TYPE m_lock;

	volatile int m_isShutDown;

	thrd_init_func m_init_func;
	thrd_beat_func m_beat_func;
	thrd_exit_func m_exit_func;
};

extern CNetPoll *g_NetPoll;

#endif 

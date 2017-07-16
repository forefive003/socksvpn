#ifndef _JOB_IO_H
#define _JOB_IO_H

#include "CThread.h"
#include "CThreadPool.h"

#define IO_FLAG_READ  0x01
#define IO_FLAG_WRITE 0x02

static void default_free_handle(int  fd, void* param1)
{
	_LOG_DEBUG("call default free func");
}

class CIoJob
{
	friend class CListenJob;
	friend class CWrIoJob;
	friend class CTcpJob;
	friend class CUdpJob;
public:
	CIoJob(int fd, void *param1) {
		m_fd = fd;
		m_param1 = param1;

		m_is_want_delete = false;
		m_now_event = 0;

		m_free_func = default_free_handle;
        _LOG_DEBUG("construct IoJob");
	}
    virtual ~CIoJob() {
        _LOG_DEBUG("destruct IoJob");
    };

    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual void read_evt_handle() = 0;
    virtual void write_evt_handle() = 0;
    virtual void set_read_callback(void *callback) = 0;
    virtual void set_write_callback(void *callback) = 0;
    virtual void init_recv_buf(int bufferSize) = 0;
	void set_free_callback(void *callback)
	{
		m_free_func = (free_hdl_func)callback;
	}
	void free_callback()
	{
		m_free_func(m_fd, m_param1);
	}

    inline int get_fd()
    {
    	return m_fd;
    }
    inline void set_deleting_flag()
    {
    	m_is_want_delete = true;
    }

	inline void add_read_io_event()
	{
		m_now_event |= IO_FLAG_READ;
	}	

	inline void del_read_io_event()
	{
		m_now_event &= ~IO_FLAG_READ;
	}

	inline void add_write_io_event()
	{
		m_now_event |= IO_FLAG_WRITE;
	}

	inline void del_write_io_event()
	{
		m_now_event &= ~IO_FLAG_WRITE;
	}

	inline BOOL io_event_read()
	{
		return (m_now_event & IO_FLAG_READ);
	}

	inline BOOL io_event_write()
	{
		return (m_now_event & IO_FLAG_WRITE);
	}

	void set_thrd_index(unsigned int thrd_index)
	{
		m_thrd_index = thrd_index;
	}
	unsigned int get_thrd_index()
	{
		return m_thrd_index;
	}
private:
	unsigned int m_thrd_index;

	int  m_fd;
	void *m_param1;
	int m_now_event;

	BOOL m_is_want_delete;
	BOOL m_isWriting;

    free_hdl_func m_free_func;
};

class CListenJob: public CIoJob
{
public:
	CListenJob(int fd, void *param1) : CIoJob(fd, param1){
	}
    virtual ~CListenJob() {
    };

public:
	void read_evt_handle();
	void set_read_callback(void *callback);
	void write_evt_handle()
	{
		_LOG_ERROR("shouldn't call write evt handle for listen job");
	}
	void set_write_callback(void *callback)
	{
		_LOG_ERROR("shouldn't set write callback for listen job");
	}
	void init_recv_buf(int bufferSize)
	{
		_LOG_ERROR("shouldn't init recv buf for listen job");
	}
	void lock()
	{
		_LOG_ERROR("shouldn't lock for listen job");
	}
    void unlock()
    {
    	_LOG_ERROR("shouldn't unlock for listen job");
    }
private:
	accept_hdl_func m_accept_func;
};

class CWrIoJob: public CIoJob
{
public:
	CWrIoJob(int fd, void *param1) : CIoJob(fd, param1){
		pthread_spin_init(&m_lock, 0);
	}
    virtual ~CWrIoJob() {
    	pthread_spin_destroy(&m_lock);
    };

public:	
	void write_evt_handle();
	void set_write_callback(void *callback);
	void lock()
	{
		pthread_spin_lock(&m_lock);
	}
    void unlock()
    {
    	pthread_spin_unlock(&m_lock);
    }

private:
	/*对于write事件, 捕获时会清除, 出现多线程同时设置的问题*/
	pthread_spinlock_t m_lock;
	write_hdl_func  m_write_func;
};

class CTcpJob: public CWrIoJob
{
public:
	CTcpJob(int fd, void *param1) : CWrIoJob(fd, param1){
		m_cache_len = 0;
		m_cache_buf = NULL;
	}
	virtual ~CTcpJob() {
    	if (NULL != m_cache_buf)
    	{
    		free(m_cache_buf);
    	}
    };

    void init_recv_buf(int bufferSize)
    {
    	m_cache_len = bufferSize;
		if (0 == m_cache_len) m_cache_len = 1500;
		m_cache_buf = (char*)malloc(m_cache_len);
		assert(m_cache_buf);
    }

public:
	void read_evt_handle();
	void set_read_callback(void *callback);
private:
	read_hdl_func  m_read_func;	
	uint32_t m_cache_len;
	char *m_cache_buf;
};

class CUdpJob: public CWrIoJob
{
public:
	CUdpJob(int fd, void *param1) : CWrIoJob(fd, param1){
		m_cache_len = 0;
		m_cache_buf = NULL;
	}
    virtual ~CUdpJob() {
    	if (NULL != m_cache_buf)
    	{
    		free(m_cache_buf);
    	}
    };

    void init_recv_buf(int bufferSize)
    {
    	m_cache_len = bufferSize;
		if (0 == m_cache_len) m_cache_len = 1500;
		m_cache_buf = (char*)malloc(m_cache_len);
		assert(m_cache_buf);
    }

public:
	void read_evt_handle();
	void set_read_callback(void *callback);
private:
	udp_read_hdl_func  m_read_func;	
	uint32_t m_cache_len;
	char *m_cache_buf;
};

#endif

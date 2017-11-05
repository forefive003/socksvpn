#ifndef _CONN_POOL_H
#define _CONN_POOL_H

#include "list.h"
#include "CNetRecv.h"

typedef struct conn_obj
{
	struct conn_obj *next;

	MUTEX_TYPE data_lock;

	int index;
	CNetRecv *connObj;	
}conn_obj_t;


#ifdef _WIN32

#if  defined(DLL_EXPORT_NP)
class _declspec(dllexport) CConnPool
#elif defined(DLL_IMPORT_NP)
class _declspec(dllimport) CConnPool
#else
class CConnPool
#endif

#else
class CConnPool
#endif

{
public:
	CConnPool(int maxConnCnt);
	virtual ~CConnPool();

public:
	int init();
	void free(); /*disconnect connection, and free*/	
	
	void lock_index(int index);
	void unlock_index(int index);

	int add_conn_obj(CNetRecv *connObj);
	void del_conn_obj(int index);
	CNetRecv* get_conn_obj(int index);

	int send_on_conn_obj(int index, char *buf, int buf_len);
	BOOL is_conn_obj_send_busy(int index);

public:
	int m_max_conn_cnt;
	conn_obj_t *m_conns_array;

private:
	void lock();
	void unlock();

	conn_obj_t *m_free_conns;
#ifdef _WIN32
	LONG m_data_lock;
#else
	pthread_spinlock_t m_data_lock;
#endif
};

#endif

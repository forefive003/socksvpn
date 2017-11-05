#ifndef _RMTSRV_CONN_POOL_H
#define _RMTSRV_CONN_POOL_H

#include "CConnPool.h"

class CRemoteServerPool : public CConnPool
{
public:
	CRemoteServerPool(int maxConnCnt) : CConnPool(maxConnCnt)
	{
		m_session_cnt = new int[maxConnCnt];

		for (int ii = 0; ii<maxConnCnt; ii++)
		{
			m_session_cnt[ii] = 0;
		}
	}

	virtual ~CRemoteServerPool()
    {
    	delete []m_session_cnt;
    }

public:
	void status_check();/*if not connected, try it; if not auth try it also*/
	void let_re_auth(); /*server changed, re auth*/

	int get_active_conn_obj();
	void print_statistic(FILE* pFd);

	void index_session_inc(int index)
	{
		this->lock_index(index);
		m_session_cnt[index]++;
		this->unlock_index(index);
	}
	void index_session_dec(int index)
	{
		this->lock_index(index);
		m_session_cnt[index]--;
		this->unlock_index(index);
	}
	int get_index_session_cnt(int index)
	{
		return m_session_cnt[index];
	}

private:
	int *m_session_cnt;
};

extern CRemoteServerPool *g_remoteSrvPool;

#endif

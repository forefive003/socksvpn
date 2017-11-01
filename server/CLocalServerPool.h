#ifndef _LOCAL_SRV_CONN_POOL_H
#define _LOCAL_SRV_CONN_POOL_H

#include "CConnPool.h"

class CLocalServerPool : public CConnPool
{
public:
	CLocalServerPool(int maxConnCnt) : CConnPool(maxConnCnt)
	{
		m_cur_index = 0;
		m_session_cnt = new int[maxConnCnt];

		for (int ii = 0; ii<maxConnCnt; ii++)
		{
			m_session_cnt[ii] = 0;
		}
	}

	virtual ~CLocalServerPool()
    {
    	delete []m_session_cnt;
    }

public:
	void status_check();/*if not connected, try it; if not auth try it also*/

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
	int m_cur_index;
	int *m_session_cnt;
};

extern CLocalServerPool *g_localSrvPool;

#endif

#ifndef _NETOBJ_POOL_H
#define _NETOBJ_POOL_H

#include "CConnPool.h"

class CNetObjPool : public CConnPool
{
public:
	CNetObjPool(int maxConnCnt) : CConnPool(maxConnCnt)
	{
		m_session_cnt = new int[maxConnCnt];

		for (int ii = 0; ii<maxConnCnt; ii++)
		{
			m_session_cnt[ii] = 0;
		}
	}

	virtual ~CNetObjPool()
    {
    	delete []m_session_cnt;
    }

public:	
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

extern CNetObjPool *g_clientNetPool;
extern CNetObjPool *g_socksNetPool;

#endif

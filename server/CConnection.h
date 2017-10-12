#ifndef _SRV_CONNECTION_H
#define _SRV_CONNECTION_H

#include "common_def.h"
#include "CBaseObj.h"
#include "CBaseConn.h"
#include "CClient.h"
#include "CRemote.h"

#define  CLI_SEND_Q_MAX_CNT  8
#define  SRV_SEND_Q_MAX_CNT  1024

class CConnection : public CBaseConnection
{
/*no extra function to be used*/
public:
    CConnection():CBaseConnection()
    {
        m_client_connect_resp_cnt = 0;
    }

public:
	int  client_send_connect_result(BOOL result)
    {
        int ret = -1;

        MUTEX_LOCK(m_remote_lock);
        if (m_client != NULL)
        {
            ret = CLIENT_CONVERT(m_client)->send_connect_result(result);
            m_client_connect_resp_cnt++;
        }
        else
        {
            _LOG_WARN("client NULL when send connect result");
        }
        MUTEX_UNLOCK(m_remote_lock);
        return ret;
    }


    void print_statistic(FILE *pFd)
    {
        MUTEX_LOCK(m_remote_lock);
        fprintf(pFd, "client-0x%x:%u inner 0x%x:%u <--> server-0x%x:%u\n",
            get_client_ipaddr(), get_client_port(), 
            get_client_inner_ipaddr(), get_client_inner_port(), 
            get_remote_ipaddr(), get_remote_port());
        fprintf(pFd, "\tconnect-resp:%"PRIu64"\t client-data:%"PRIu64"\t remote-data:%"PRIu64"\n", 
            m_client_connect_resp_cnt, 
            m_send_client_data_cnt, m_send_remote_data_cnt);
        MUTEX_UNLOCK(m_remote_lock);

        m_client_connect_resp_cnt = 0;
        m_send_client_data_cnt = 0;
        m_send_remote_data_cnt = 0;
    }

public:
    uint64_t m_client_connect_resp_cnt;
};

#endif

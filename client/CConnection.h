#ifndef _CLI_CONNECTION_H
#define _CLI_CONNECTION_H

#include "common_def.h"
#include "CBaseObj.h"
#include "CBaseConn.h"
#include "CClient.h"
#include "CRemote.h"

class CConnection : public CBaseConnection
{
public:
    CConnection():CBaseConnection()
    {
        m_client_connect_resp_cnt = 0;
        m_client_connect_req_cnt = 0;
    }
public:
    int fwd_client_connect_msg(char *buf, int buf_len)
    {
        int ret = -1;

        MUTEX_LOCK(m_remote_lock);
        if (m_remote != NULL)
        {
            ret = REMOTE_CONVERT(m_remote)->send_client_connect_msg(buf, buf_len);
            m_client_connect_req_cnt++;
        }
        else
        {
            _LOG_WARN("remote NULL when forward client connect msg");
        }
        MUTEX_UNLOCK(m_remote_lock);
        
        return ret;
    }

public:
    int get_client_status()
    {
        int ret = CLI_INIT;
        MUTEX_LOCK(m_remote_lock);
        if (m_client != NULL)
        {
            ret = CLIENT_CONVERT(m_client)->get_client_status();
        }
        else
        {
            _LOG_WARN("client NULL when get client status");
        }
        MUTEX_UNLOCK(m_remote_lock);
        
        return ret;
    }

    void set_client_status(CLI_STATUS_E status)
    {
        MUTEX_LOCK(m_remote_lock);
        if (m_client != NULL)
        {
            CLIENT_CONVERT(m_client)->set_client_status(status);
        }
        else
        {
            _LOG_WARN("client NULL when set client status");
        }
        MUTEX_UNLOCK(m_remote_lock);
        
    }

    void client_auth_result_handle(BOOL result)
    {
        MUTEX_LOCK(m_remote_lock);
        if (m_client != NULL)
        {
            CLIENT_CONVERT(m_client)->auth_result_handle(result);
        }
        else
        {
            _LOG_WARN("client NULL when handle auth result");
        }
        MUTEX_UNLOCK(m_remote_lock);
        
    }

    void client_connect_result_handle(BOOL result)
    {
        m_client_connect_resp_cnt++;

        MUTEX_LOCK(m_remote_lock);
        if (m_client != NULL)
        {
            CLIENT_CONVERT(m_client)->connect_result_handle(result);
        }
        else
        {
            _LOG_WARN("client NULL when handle connect result");
        }
        MUTEX_UNLOCK(m_remote_lock);
    }


    void print_statistic(FILE *pFd)
    {
        MUTEX_LOCK(m_remote_lock);
        fprintf(pFd, "client-0x%x:%u inner 0x%x:%u <--> server-0x%x:%u\n",
            get_client_ipaddr(), get_client_port(), 
            get_client_inner_ipaddr(), get_client_inner_port(), 
            get_remote_ipaddr(), get_remote_port());
        fprintf(pFd, "\tconnect-req:%"PRIu64"\t connect-resp:%"PRIu64"\t client-data:%"PRIu64"\t remote-data:%"PRIu64"\n", 
            m_client_connect_req_cnt, m_client_connect_resp_cnt, 
            m_send_client_data_cnt, m_send_remote_data_cnt);
        MUTEX_UNLOCK(m_remote_lock);
#if 1
        m_client_connect_resp_cnt = 0;
        m_client_connect_req_cnt = 0;
        m_send_client_data_cnt = 0;
        m_send_remote_data_cnt = 0;
#endif
    }
public:
    uint64_t m_client_connect_resp_cnt;
    uint64_t m_client_connect_req_cnt;
};

#endif

#ifndef _RELAY_CONNECTION_H
#define _RELAY_CONNECTION_H

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

            m_request_time = util_get_cur_time();
        }
        else
        {
            _LOG_WARN("remote NULL when client connect msg");
        }
        MUTEX_UNLOCK(m_remote_lock);
        
        return ret;
    }

public:
    int fwd_remote_connect_result_msg(char *buf, int buf_len)
    {
        int ret = -1;
        MUTEX_LOCK(m_remote_lock);
        if (m_client != NULL)
        {
            uint64_t response_time = util_get_cur_time();
            uint64_t consume_time = response_time - this->m_request_time;
            _LOG_INFO("client %s/%u connect remote consume %ds", m_client->m_ipstr, m_client->m_port, consume_time);

            ret = CLIENT_CONVERT(m_client)->send_connect_result_msg(buf, buf_len);
            m_client_connect_resp_cnt++;
        }
        else
        {
            _LOG_WARN("client NULL when fwd remote connect result msg");
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
        fprintf(pFd, "\tconnect-req:%"PRIu64"\t connect-resp:%"PRIu64"\t client-data:%"PRIu64"\t remote-data:%"PRIu64"\n", 
            m_client_connect_req_cnt, m_client_connect_resp_cnt,
            m_send_client_data_cnt, m_send_remote_data_cnt);
        MUTEX_UNLOCK(m_remote_lock);

        m_client_connect_resp_cnt = 0;
        m_client_connect_req_cnt = 0;
        m_send_client_data_cnt = 0;
        m_send_remote_data_cnt = 0;
    }

public:
    uint64_t m_request_time;
    uint64_t m_client_connect_resp_cnt;
    uint64_t m_client_connect_req_cnt;
};

#endif

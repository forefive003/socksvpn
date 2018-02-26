#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CRemote.h"
#include "socks_server.h"

int CRemote::connect_handle(BOOL result)
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    uint64_t response_time = util_get_cur_time();
    uint64_t consume_time = response_time - this->m_request_time;
    g_total_connect_resp_consume_time += consume_time;
    g_total_connect_resp_cnt++;
    _LOG_INFO("connect remote %s/%u consume %ds", m_ipstr, m_port, consume_time);
    if (this->m_request_time == 0)
    {
        _LOG_ERROR("no request time when connect remote %s/%u", m_ipstr, m_port);
    }

    if (!result)
    {
        _LOG_WARN("fail to connect to remote %s/%u", m_ipstr, m_port);
        pConn->client_send_connect_result(false);
        /*先发送报文*/
        pConn->notify_remote_close();
        /*在释放client*/
        pConn->free_client();
        return 0;
    }
    
    g_total_remote_cnt++;

    if(0 != this->register_read())
    {
        return -1;
    }
    else
    {
        pConn->client_send_connect_result(true);
    }
    return 0;
}

int CRemote::send_data_msg(char *buf, int buf_len)
{
	return send_data(buf, buf_len);
}

int CRemote::recv_handle(char *buf, int buf_len)
{
    return m_owner_conn->fwd_remote_data_msg(buf, buf_len);
}


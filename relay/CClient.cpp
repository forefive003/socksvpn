
#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CClient.h"
#include "CRemote.h"
#include "CClientNet.h"
#include "CSocksSrv.h"
#include "CNetObjPool.h"
#include "CNetObjSet.h"
#include "CNetObjMgr.h"

#include "socks_relay.h"
#include "CWebApi.h"
#include "relay_pkt_def.h"

int CClient::send_remote_close_msg()
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    PKT_HDR_T pkthdr;
    PKT_R2C_HDR_T r2chdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&r2chdr, 0, sizeof(PKT_R2C_HDR_T));

    CClientNet *clientNet = NULL;

    pkthdr.pkt_type = PKT_R2C;
    pkthdr.pkt_len = sizeof(PKT_R2C_HDR_T);
    PKT_HDR_HTON(&pkthdr);

    r2chdr.server_ip = pConn->get_remote_ipaddr();
    r2chdr.server_port = pConn->get_remote_port();
    r2chdr.client_ip = this->m_inner_ipaddr;
    r2chdr.client_port = this->m_inner_port;
    r2chdr.sub_type = REMOTE_CLOSED;
    r2chdr.reserved = 0;
    PKT_R2C_HDR_HTON(&r2chdr);

    g_clientNetPool->lock_index(m_client_srv_index);
    clientNet = (CClientNet*)g_clientNetPool->get_conn_obj(m_client_srv_index);
    if (NULL == clientNet)
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        _LOG_WARN("fail to find clientserver by %s/%u when send connect result to client", m_ipstr, m_port);
        return -1;
    }
    if(0 != clientNet->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        return -1;
    }
    if(0 != clientNet->send_data((char*)&r2chdr, sizeof(PKT_R2C_HDR_T)))
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        return -1;
    }

    clientNet->m_send_remote_close_cnt++;
    g_clientNetPool->unlock_index(m_client_srv_index);

    _LOG_INFO("client(%s/%u/%s/%u/fd%d) send remote close msg to client", 
        m_ipstr, m_port,
        m_inner_ipstr, m_inner_port, m_fd);
    return 0;
}

int CClient::send_connect_result_msg(char *buf, int buf_len)
{
    uint64_t response_time = util_get_cur_time();
    uint64_t consume_time = response_time - this->m_request_time;
    g_total_connect_resp_consume_time += consume_time;
    g_total_connect_resp_cnt++;

    
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    PKT_HDR_T pkthdr;
    PKT_R2C_HDR_T r2chdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&r2chdr, 0, sizeof(PKT_R2C_HDR_T));

    CClientNet *clientNet = NULL;

    pkthdr.pkt_type = PKT_R2C;
    pkthdr.pkt_len = sizeof(PKT_R2C_HDR_T) + buf_len;
    PKT_HDR_HTON(&pkthdr);

    r2chdr.server_ip = pConn->get_remote_ipaddr();
    r2chdr.server_port = pConn->get_remote_port();
    r2chdr.client_ip = this->m_inner_ipaddr;
    r2chdr.client_port = this->m_inner_port;
    r2chdr.sub_type = R2C_CONNECT_RESULT;
    r2chdr.reserved = 0;
    PKT_R2C_HDR_HTON(&r2chdr);

    g_clientNetPool->lock_index(m_client_srv_index);
    clientNet = (CClientNet*)g_clientNetPool->get_conn_obj(m_client_srv_index);
    if (NULL == clientNet)
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        _LOG_WARN("fail to find clientserver by %s/%u when send connect result to client", m_ipstr, m_port);
        return -1;
    }

    if(0 != clientNet->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        return -1;
    }
    if(0 != clientNet->send_data((char*)&r2chdr, sizeof(PKT_R2C_HDR_T)))
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        return -1;
    }
    if(0 != clientNet->send_data(buf, buf_len))
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        return -1;
    }

    clientNet->m_send_connect_result_cnt++;
    g_clientNetPool->unlock_index(m_client_srv_index);

    _LOG_INFO("client(%s/%u/%s/%u/fd%d) send connect result msg to client",
        m_ipstr, m_port, 
        m_inner_ipstr, m_inner_port, m_fd);
    return 0;
}

int CClient::send_data_msg(char *buf, int buf_len)
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    PKT_HDR_T pkthdr;
    PKT_R2C_HDR_T r2chdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&r2chdr, 0, sizeof(PKT_R2C_HDR_T));

    CClientNet *clientNet = NULL;

    pkthdr.pkt_type = PKT_R2C;
    pkthdr.pkt_len = sizeof(PKT_R2C_HDR_T) + buf_len;
    PKT_HDR_HTON(&pkthdr);

    r2chdr.server_ip = pConn->get_remote_ipaddr();
    r2chdr.server_port = pConn->get_remote_port();
    r2chdr.client_ip = this->m_inner_ipaddr;
    r2chdr.client_port = this->m_inner_port;
    r2chdr.sub_type = R2C_DATA;
    r2chdr.reserved = 0;
    PKT_R2C_HDR_HTON(&r2chdr);

    g_clientNetPool->lock_index(m_client_srv_index);
    clientNet = (CClientNet*)g_clientNetPool->get_conn_obj(m_client_srv_index);
    if (NULL == clientNet)
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        _LOG_WARN("fail to find clientserver by %s/%u when send data to client", m_ipstr, m_port);
        return -1;
    }

    if(0 != clientNet->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        return -1;
    }
    if(0 != clientNet->send_data((char*)&r2chdr, sizeof(PKT_R2C_HDR_T)))
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        return -1;
    }
    if(0 != clientNet->send_data(buf, buf_len))
    {
        g_clientNetPool->unlock_index(m_client_srv_index);
        return -1;
    }

    clientNet->m_send_data_cnt++;
    g_clientNetPool->unlock_index(m_client_srv_index);
    
    return 0;
}

#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "CClient.h"
#include "CRemote.h"
#include "CLocalServer.h"
#include "CLocalServerPool.h"
#include "socks_server.h"

int CClient::send_remote_close_msg()
{
    /*nothing to send, just close remote in free_handle*/
    PKT_HDR_T pkthdr;
    PKT_S2R_HDR_T s2rhdr;

    memset(&pkthdr, 0, sizeof(pkthdr));
    memset(&s2rhdr, 0, sizeof(s2rhdr));

    pkthdr.pkt_type = PKT_S2R;
    pkthdr.pkt_len = sizeof(PKT_S2R_HDR_T);
    PKT_HDR_HTON(&pkthdr);

    s2rhdr.client_pub_ip = m_ipaddr;
    s2rhdr.client_pub_port = m_port;
    s2rhdr.client_inner_ip = m_inner_ipaddr;
    s2rhdr.client_inner_port = m_inner_port;
    s2rhdr.sub_type = REMOTE_CLOSED;
    PKT_S2R_HDR_HTON(&s2rhdr);

    CLocalServer *localSrv = NULL;

    g_localSrvPool->lock_index(m_local_srv_index);
    localSrv = (CLocalServer*)g_localSrvPool->get_conn_obj(m_local_srv_index);
    if (NULL == localSrv)
    {
        g_localSrvPool->unlock_index(m_local_srv_index);
        _LOG_WARN("fail to find clientserver by %s/%u when send connect result to client", m_ipstr, m_port);
        return -1;
    }

    if((0 != localSrv->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
        || (0 != localSrv->send_data((char*)&s2rhdr, sizeof(PKT_S2R_HDR_T))))
    {
        g_localSrvPool->unlock_index(m_local_srv_index);

        _LOG_ERROR("client(%s/%u/%s/%u/fd%d) send remote close msg to client failed", m_ipstr, m_port, 
            m_inner_ipstr, m_inner_port, m_fd);
        return -1;
    }

    if (localSrv->is_send_busy())
    {
        this->m_owner_conn->set_client_busy(true);
    }
    g_localSrvPool->unlock_index(m_local_srv_index);

    _LOG_INFO("client(%s/%u/%s/%u/fd%d) send remote close msg to client", m_ipstr, m_port, 
        m_inner_ipstr, m_inner_port, m_fd);
    return 0;
}

int CClient::send_connect_result(BOOL result)
{
	char resp_buf[10] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int resp_buf_len = 10;
    
    if (!result)
    {
        /*set flag, which should be terminated in client, can't forward to app by client
        because this action is not standard*/
        resp_buf[1] = 0xFF;
    }
    else
    {
        /*last four bytes to fill remote ipaddr*/
        CConnection *pConn = (CConnection*)this->m_owner_conn;
        *((int*)(&resp_buf[6])) = htonl(pConn->m_remote->m_ipaddr);
    }

	PKT_HDR_T pkthdr;
    PKT_S2R_HDR_T s2rhdr;

    memset(&pkthdr, 0, sizeof(pkthdr));
    memset(&s2rhdr, 0, sizeof(s2rhdr));

    pkthdr.pkt_type = PKT_S2R;
    pkthdr.pkt_len = sizeof(PKT_S2R_HDR_T) + resp_buf_len;
    PKT_HDR_HTON(&pkthdr);

    s2rhdr.client_pub_ip = m_ipaddr;
    s2rhdr.client_pub_port = m_port;
    s2rhdr.client_inner_ip = m_inner_ipaddr;
    s2rhdr.client_inner_port = m_inner_port;
    s2rhdr.sub_type = S2R_CONNECT_RESULT;
    PKT_S2R_HDR_HTON(&s2rhdr);

    CLocalServer *localSrv = NULL;

    g_localSrvPool->lock_index(m_local_srv_index);
    localSrv = (CLocalServer*)g_localSrvPool->get_conn_obj(m_local_srv_index);
    if (NULL == localSrv)
    {
        g_localSrvPool->unlock_index(m_local_srv_index);
        _LOG_WARN("fail to find clientserver by %s/%u when send connect result to client", m_ipstr, m_port);
        return -1;
    }

    if((0 != localSrv->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
        || (0 != localSrv->send_data((char*)&s2rhdr, sizeof(PKT_S2R_HDR_T)))
        || (0 != localSrv->send_data(resp_buf, sizeof(resp_buf))))
    {
        g_localSrvPool->unlock_index(m_local_srv_index);

        _LOG_ERROR("client(%s/%u/%s/%u/fd%d) send connect result to client failed", m_ipstr, m_port, 
                m_inner_ipstr, m_inner_port, m_fd);
        return -1;
    }

    if (localSrv->is_send_busy())
    {
        this->m_owner_conn->set_client_busy(true);
    }

    g_localSrvPool->unlock_index(m_local_srv_index);

    _LOG_INFO("client(%s/%u/%s/%u/fd%d) send connect result to client", m_ipstr, m_port, 
        m_inner_ipstr, m_inner_port, m_fd);
    return 0;
}

int CClient::send_data_msg(char *buf, int buf_len)
{
	PKT_HDR_T pkthdr;
    PKT_S2R_HDR_T s2rhdr;

    memset(&pkthdr, 0, sizeof(pkthdr));
    memset(&s2rhdr, 0, sizeof(s2rhdr));

    pkthdr.pkt_type = PKT_S2R;
    pkthdr.pkt_len = sizeof(PKT_S2R_HDR_T) + buf_len;
    PKT_HDR_HTON(&pkthdr);

    s2rhdr.client_pub_ip = m_ipaddr;
    s2rhdr.client_pub_port = m_port;
    s2rhdr.client_inner_ip = m_inner_ipaddr;
    s2rhdr.client_inner_port = m_inner_port;
    s2rhdr.sub_type = S2R_DATA;
    PKT_S2R_HDR_HTON(&s2rhdr);

    CLocalServer *localSrv = NULL;

    g_localSrvPool->lock_index(m_local_srv_index);
    localSrv = (CLocalServer*)g_localSrvPool->get_conn_obj(m_local_srv_index);
    if (NULL == localSrv)
    {
        g_localSrvPool->unlock_index(m_local_srv_index);
        _LOG_WARN("fail to find clientserver by %s/%u when send connect result to client", m_ipstr, m_port);
        return -1;
    }

    if((0 != localSrv->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
        || (0 != localSrv->send_data((char*)&s2rhdr, sizeof(PKT_S2R_HDR_T)))
        || (0 != localSrv->send_data(buf, buf_len)))
    {
        g_localSrvPool->unlock_index(m_local_srv_index);
        return -1;
    }

    if (localSrv->is_send_busy())
    {
        this->m_owner_conn->set_client_busy(true);
    }

    g_localSrvPool->unlock_index(m_local_srv_index);

    return 0;
}

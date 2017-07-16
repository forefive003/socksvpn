#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "common_def.h"
#include "CClient.h"
#include "CRemote.h"
#include "CRemoteServer.h"
#include "socks_client.h"

int CRemote::send_client_close_msg()
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    PKT_HDR_T pkthdr;
    PKT_C2R_HDR_T c2rhdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&c2rhdr, 0, sizeof(PKT_C2R_HDR_T));

    pkthdr.pkt_type = PKT_C2R;
    pkthdr.pkt_len = sizeof(PKT_C2R_HDR_T);

    c2rhdr.server_ip = g_server_ip;
    c2rhdr.client_ip = pConn->get_client_ipaddr();
    c2rhdr.server_port = 0;
    c2rhdr.client_port = pConn->get_client_port();
    c2rhdr.sub_type = CLIENT_CLOSED;
    c2rhdr.reserved = 0;

    MUTEX_LOCK(m_remote_srv_lock);
    if (NULL == g_RemoteServ)
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        _LOG_WARN("remote server NULL when send data.");
        return -1;
    }
    if(0 != g_RemoteServ->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return -1;
    }
    if(0 != g_RemoteServ->send_data((char*)&c2rhdr, sizeof(PKT_C2R_HDR_T)))
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return -1;
    }
    MUTEX_UNLOCK(m_remote_srv_lock);

    _LOG_INFO("client(0x%x/%u/fd%d) send client close msg to remote", 
        pConn->get_client_ipaddr(), pConn->get_client_port(), pConn->get_client_fd());
    return 0;
}

int CRemote::send_client_connect_msg(char *buf, int buf_len)
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    PKT_HDR_T pkthdr;
    PKT_C2R_HDR_T c2rhdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&c2rhdr, 0, sizeof(PKT_C2R_HDR_T));

    pkthdr.pkt_type = PKT_C2R;
    pkthdr.pkt_len = sizeof(PKT_C2R_HDR_T) + buf_len;

    c2rhdr.server_ip = g_server_ip;
    c2rhdr.client_ip = pConn->get_client_ipaddr();
    c2rhdr.server_port = 0;
    c2rhdr.client_port = pConn->get_client_port();
    c2rhdr.sub_type = C2R_CONNECT;
    c2rhdr.reserved = 0;

    MUTEX_LOCK(m_remote_srv_lock);
    if (NULL == g_RemoteServ)
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        _LOG_WARN("remote server NULL when send data.");
        return -1;
    }
    if(0 != g_RemoteServ->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return -1;
    }
    if(0 != g_RemoteServ->send_data((char*)&c2rhdr, sizeof(PKT_C2R_HDR_T)))
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return -1;
    }
    if(0 != g_RemoteServ->send_data(buf, buf_len))
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return -1;
    }
    MUTEX_UNLOCK(m_remote_srv_lock);

    _LOG_INFO("client(0x%x/%u/fd%d) send connect msg to remote", 
        pConn->get_client_ipaddr(), pConn->get_client_port(), pConn->get_client_fd());
    return 0;
}

int CRemote::send_data_msg(char *buf, int buf_len)
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    PKT_HDR_T pkthdr;
    PKT_C2R_HDR_T c2rhdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&c2rhdr, 0, sizeof(PKT_C2R_HDR_T));

    pkthdr.pkt_type = PKT_C2R;
    pkthdr.pkt_len = sizeof(PKT_C2R_HDR_T) + buf_len;

    c2rhdr.server_ip = g_server_ip;
    c2rhdr.client_ip = pConn->get_client_ipaddr();
    c2rhdr.server_port = 0;
    c2rhdr.client_port = pConn->get_client_port();
    c2rhdr.sub_type = C2R_DATA;
    c2rhdr.reserved = 0;

    MUTEX_LOCK(m_remote_srv_lock);
    if (NULL == g_RemoteServ)
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        _LOG_WARN("remote server NULL when send data.");        
        return -1;
    }
    if(0 != g_RemoteServ->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return -1;
    }
    if(0 != g_RemoteServ->send_data((char*)&c2rhdr, sizeof(PKT_C2R_HDR_T)))
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return -1;
    }
    if(0 != g_RemoteServ->send_data(buf, buf_len))
    {
        MUTEX_UNLOCK(m_remote_srv_lock);
        return -1;
    }
    MUTEX_UNLOCK(m_remote_srv_lock);

    return 0;
}

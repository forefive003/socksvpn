#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CClient.h"
#include "CRemote.h"
#include "socks_relay.h"
#include "CSocksSrvMgr.h"

int CRemote::send_client_close_msg()
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;

    CSocksSrv *socksSrv = NULL;

    g_SocksSrvMgr->lock();
    socksSrv = g_SocksSrvMgr->get_socks_server_by_user(m_ipaddr, m_username);
    if (NULL == socksSrv)
    {
        g_SocksSrvMgr->unlock();
        _LOG_ERROR("fail to find socksserver by 0x%x, username %s", m_ipaddr, m_username);
        return -1;
    }

    PKT_HDR_T pkthdr;
    PKT_R2S_HDR_T r2shdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&r2shdr, 0, sizeof(PKT_R2S_HDR_T));

    pkthdr.pkt_type = PKT_R2S;
    pkthdr.pkt_len = sizeof(PKT_R2S_HDR_T);
    PKT_HDR_HTON(&pkthdr);

    r2shdr.client_pub_ip = pConn->get_client_ipaddr();
    r2shdr.client_pub_port = pConn->get_client_port();
    r2shdr.client_inner_ip = pConn->get_client_inner_ipaddr();
    r2shdr.client_inner_port = pConn->get_client_inner_port();
    r2shdr.sub_type = CLIENT_CLOSED;
    PKT_R2S_HDR_HTON(&r2shdr);

    if(0 != socksSrv->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        g_SocksSrvMgr->unlock();
        return -1;
    }
    if(0 != socksSrv->send_data((char*)&r2shdr, sizeof(PKT_R2S_HDR_T)))
    {
        g_SocksSrvMgr->unlock();
        return -1;
    }
    socksSrv->m_send_client_close_cnt++;
    g_SocksSrvMgr->unlock();

    _LOG_INFO("client(0x%x/%u/0x%x/%u/fd%d) send client close msg",
        pConn->get_client_ipaddr(), pConn->get_client_port(), 
        pConn->get_client_inner_ipaddr(), pConn->get_client_inner_port(),
        pConn->get_client_fd());
    
    return 0;
}

int CRemote::send_client_connect_msg(char *buf, int buf_len)
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;
    CSocksSrv *socksSrv = NULL;

    g_SocksSrvMgr->lock();
    socksSrv = g_SocksSrvMgr->get_socks_server_by_user(m_ipaddr, m_username);
    if (NULL == socksSrv)
    {
        g_SocksSrvMgr->unlock();
        _LOG_ERROR("fail to find socksserver by 0x%x, username %s when send client connect msg", m_ipaddr, m_username);
        return -1;
    }

    PKT_HDR_T pkthdr;
    PKT_R2S_HDR_T r2shdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&r2shdr, 0, sizeof(PKT_R2S_HDR_T));

    pkthdr.pkt_type = PKT_R2S;
    pkthdr.pkt_len = sizeof(PKT_R2S_HDR_T) + buf_len;
    PKT_HDR_HTON(&pkthdr);

    r2shdr.client_pub_ip = pConn->get_client_ipaddr();
    r2shdr.client_pub_port = pConn->get_client_port();
    r2shdr.client_inner_ip = pConn->get_client_inner_ipaddr();
    r2shdr.client_inner_port = pConn->get_client_inner_port();
    r2shdr.sub_type = R2S_CLIENT_CONNECT;
    PKT_R2S_HDR_HTON(&r2shdr);

    if(0 != socksSrv->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        g_SocksSrvMgr->unlock();
        return -1;
    }
    if(0 != socksSrv->send_data((char*)&r2shdr, sizeof(PKT_R2S_HDR_T)))
    {
        g_SocksSrvMgr->unlock();
        return -1;
    }
    if(0 != socksSrv->send_data(buf, buf_len))
    {
        g_SocksSrvMgr->unlock();
        return -1;
    }
    socksSrv->m_send_client_connect_cnt++;
    g_SocksSrvMgr->unlock();    

    _LOG_INFO("client(0x%x/%u/0x%x/%u/fd%d) send connect msg to remote, use fd %d",
        pConn->get_client_ipaddr(), pConn->get_client_port(), 
        pConn->get_client_inner_ipaddr(), pConn->get_client_inner_port(),
        pConn->get_client_fd(),
        socksSrv->m_fd);
    return 0;
}

int CRemote::send_data_msg(char *buf, int buf_len)
{
    CConnection *pConn = (CConnection*)this->m_owner_conn;
    CSocksSrv *socksSrv = NULL;

    g_SocksSrvMgr->lock();
    socksSrv = g_SocksSrvMgr->get_socks_server_by_user(m_ipaddr, m_username);
    if (NULL == socksSrv)
    {
        g_SocksSrvMgr->unlock();
        _LOG_ERROR("fail to find socksserver by 0x%x, username %s", m_ipaddr, m_username);
        return -1;
    }

    PKT_HDR_T pkthdr;
    PKT_R2S_HDR_T r2shdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&r2shdr, 0, sizeof(PKT_R2S_HDR_T));

    pkthdr.pkt_type = PKT_R2S;
    pkthdr.pkt_len = sizeof(PKT_R2S_HDR_T) + buf_len;
    PKT_HDR_HTON(&pkthdr);

    r2shdr.client_pub_ip = pConn->get_client_ipaddr();
    r2shdr.client_pub_port = pConn->get_client_port();
    r2shdr.client_inner_ip = pConn->get_client_inner_ipaddr();
    r2shdr.client_inner_port = pConn->get_client_inner_port();
    r2shdr.sub_type = R2S_DATA;
    PKT_R2S_HDR_HTON(&r2shdr);

    if(0 != socksSrv->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        g_SocksSrvMgr->unlock();
        return -1;
    }
    if(0 != socksSrv->send_data((char*)&r2shdr, sizeof(PKT_R2S_HDR_T)))
    {
        g_SocksSrvMgr->unlock();
        return -1;
    }
    if(0 != socksSrv->send_data(buf, buf_len))
    {
        g_SocksSrvMgr->unlock();
        return -1;
    }
    socksSrv->m_send_data_cnt++;
    g_SocksSrvMgr->unlock();
    return 0;
}

void CRemote::set_username(char *username)
{
    memset(m_username, 0, sizeof(m_username));
    strncpy(m_username, username, MAX_USERNAME_LEN);
}

#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CClient.h"
#include "CSocksSrv.h"
#include "CSocksSrvMgr.h"
#include "socks_relay.h"
#include "CSocksMem.h"
#include "CWebApi.h"

int CSocksSrv::msg_srv_reg_handle(char *data_buf, int data_len)
{
    PKT_SRV_REG_T *srvReg = (PKT_SRV_REG_T*)data_buf;
    CSocksSrv *socksSrv = NULL;

    this->m_recv_srv_alive_cnt++;

    if (1 == srvReg->is_keepalive)
    {
        g_SocksSrvMgr->lock();
        socksSrv = g_SocksSrvMgr->get_socks_server_by_innnerip(m_ipaddr, srvReg->local_ip);
        if (NULL == socksSrv)
        {
            g_SocksSrvMgr->unlock();
            _LOG_ERROR("fail to find socksserver by 0x%x, localip 0x%x", m_ipaddr, srvReg->local_ip);
            return -1;
        }

        _LOG_DEBUG("update keepalive for socksserver 0x%x, localip 0x%x", m_ipaddr, srvReg->local_ip);
        g_SocksSrvMgr->unlock();
    }
    else
    {
        _LOG_INFO("recv reg from socksserver 0x%x, inner ip 0x%x", m_ipaddr, srvReg->local_ip);

        g_SocksSrvMgr->lock();
        socksSrv = g_SocksSrvMgr->get_socks_server_by_innnerip(m_ipaddr, srvReg->local_ip);
        if (NULL != socksSrv)
        {
            g_SocksSrvMgr->unlock();
            _LOG_WARN("socksserver 0x%x, localip 0x%x alreay exist", m_ipaddr, srvReg->local_ip);
            return 0;
        }
        g_SocksSrvMgr->unlock();

        this->set_inner_info(srvReg->local_ip, srvReg->local_port);

        /*获取配置*/
        if (srvReg->sn[0] != 0)
        {
            if(0 != g_SrvCfgMgr->get_server_cfg(srvReg->sn, &this->m_srvCfg))
            {
                _LOG_WARN("socksserver 0x%x, localip 0x%x failed to get srv config", m_ipaddr, srvReg->local_ip);
            }
            /*通知平台*/
            if(0 != g_webApi->postServerOnline(srvReg->sn, m_ipstr, m_inner_ipstr, true))
            {
                _LOG_WARN("socksserver 0x%x, localip 0x%x failed to post platform", m_ipaddr, srvReg->local_ip);
            }
        }
    }

    this->m_update_time = util_get_cur_time();
    return 0;
}

int CSocksSrv::msg_data_handle(PKT_S2R_HDR_T *s2rhdr, char *data_buf, int data_len)
{
    struct sockaddr_storage ipaddr;
    char client_ipstr[HOST_IP_LEN] = { 0 };

    this->m_recv_data_cnt++;

    memset((void*) &ipaddr, 0, sizeof(struct sockaddr_storage));
    ipaddr.ss_family = AF_INET;
    ((struct sockaddr_in *) &ipaddr)->sin_addr.s_addr = htonl(s2rhdr->client_pub_ip);

    if (NULL == util_ip_to_str(&ipaddr, client_ipstr)) 
    {
        _LOG_ERROR("ip to str failed.");
        return -1;
    }

    CConnection *pConn = (CConnection*)g_ConnMgr->get_conn_by_client(s2rhdr->client_pub_ip, 
                                s2rhdr->client_pub_port,
                                s2rhdr->client_inner_ip,
                                s2rhdr->client_inner_port);
    if (NULL == pConn)
    {
        _LOG_WARN("failed to find connection for client %s/%u, inner 0x%x/%u, when recv server data",  
                client_ipstr, s2rhdr->client_pub_port,
                s2rhdr->client_inner_ip, s2rhdr->client_inner_port);
        return -1;
    }

    /*发送给远程*/
    if (0 != pConn->fwd_remote_data_msg(data_buf, data_len))
    {
        _LOG_ERROR("send to remote failed for client %s/%u, inner 0x%x/%u",  
                client_ipstr, s2rhdr->client_pub_port,
                s2rhdr->client_inner_ip, s2rhdr->client_inner_port);
        pConn->dec_ref();
        return -1;
    }

    pConn->dec_ref();
    return 0;
}

int CSocksSrv::msg_connect_result_handle(PKT_S2R_HDR_T *s2rhdr, char *data_buf, int data_len)
{
    struct sockaddr_storage ipaddr;
    char client_ipstr[HOST_IP_LEN] = { 0 };

    this->m_recv_connect_result_cnt++;

    memset((void*) &ipaddr, 0, sizeof(struct sockaddr_storage));
    ipaddr.ss_family = AF_INET;
    ((struct sockaddr_in *) &ipaddr)->sin_addr.s_addr = htonl(s2rhdr->client_pub_ip);

    if (NULL == util_ip_to_str(&ipaddr, client_ipstr)) 
    {
        _LOG_ERROR("ip to str failed.");
        return -1;
    }

    CConnection *pConn = (CConnection*)g_ConnMgr->get_conn_by_client(s2rhdr->client_pub_ip, 
                                s2rhdr->client_pub_port,
                                s2rhdr->client_inner_ip,
                                s2rhdr->client_inner_port);
    if (NULL == pConn)
    {
        _LOG_WARN("failed to find connection for client %s/%u, inner 0x%x/%u, when recv connect resp",  
                client_ipstr, s2rhdr->client_pub_port,
                s2rhdr->client_inner_ip, s2rhdr->client_inner_port);
        return -1;
    }

    _LOG_INFO("recv connect response from remote");

    /*发送给远程*/
    if (0 != pConn->fwd_remote_connect_result_msg(data_buf, data_len))
    {
        _LOG_ERROR("send to remote failed for  client %s/%u, inner 0x%x/%u",  
                client_ipstr, s2rhdr->client_pub_port,
                s2rhdr->client_inner_ip, s2rhdr->client_inner_port);
        pConn->dec_ref();
        return -1;
    }

    pConn->dec_ref();
    return 0;
}

int CSocksSrv::msg_remote_closed_handle(PKT_S2R_HDR_T *s2rhdr, char *data_buf, int data_len)
{
    /*通知client关闭*/
    struct sockaddr_storage ipaddr;
    char client_ipstr[HOST_IP_LEN] = { 0 };

    this->m_recv_remote_close_cnt++;

    memset((void*) &ipaddr, 0, sizeof(struct sockaddr_storage));
    ipaddr.ss_family = AF_INET;
    ((struct sockaddr_in *) &ipaddr)->sin_addr.s_addr = htonl(s2rhdr->client_pub_ip);

    if (NULL == util_ip_to_str(&ipaddr, client_ipstr)) 
    {
        _LOG_ERROR("ip to str failed.");
        return -1;
    }

    CConnection *pConn = (CConnection*)g_ConnMgr->get_conn_by_client(s2rhdr->client_pub_ip, 
                                s2rhdr->client_pub_port,
                                s2rhdr->client_inner_ip,
                                s2rhdr->client_inner_port);
    if (NULL == pConn)
    {
        _LOG_WARN("failed to find connection for client %s/%u, inner 0x%x/%u, when recv server close",  
                client_ipstr, s2rhdr->client_pub_port,
                s2rhdr->client_inner_ip, s2rhdr->client_inner_port);
        return -1;
    }

    _LOG_INFO("recv remote closed msg on client(0x%x/%u/0x%x/%u)",
                pConn->get_client_ipaddr(), pConn->get_client_port(),
                pConn->get_client_inner_ipaddr(), pConn->get_client_inner_port());

    MUTEX_LOCK(pConn->m_remote_lock);
    if (pConn->m_remote != NULL)
    {
        pConn->m_remote->free();
    }
    MUTEX_UNLOCK(pConn->m_remote_lock);

    pConn->dec_ref();
    return 0;
}

int CSocksSrv::pdu_handle(char *pdu_buf, int pdu_len)
{
    int ret = 0;

    char *data_buf = NULL;
    int data_len = 0;

    PKT_HDR_T *pkthdr = (PKT_HDR_T*)pdu_buf;
    if (pkthdr->pkt_type == PKT_S2R_REG)
    {
        data_buf = pdu_buf + sizeof(PKT_HDR_T);
        data_len = pkthdr->pkt_len;
        ret = this->msg_srv_reg_handle(data_buf, data_len);
    }
    else if (pkthdr->pkt_type == PKT_S2R)
    {
        PKT_S2R_HDR_T *s2rhdr = (PKT_S2R_HDR_T*)(pdu_buf + sizeof(PKT_HDR_T));

        data_buf = pdu_buf + sizeof(PKT_HDR_T) + sizeof(PKT_S2R_HDR_T);
        data_len = pkthdr->pkt_len - sizeof(PKT_S2R_HDR_T);

        switch(s2rhdr->sub_type)
        {
            case S2R_CONNECT_RESULT:
                ret = msg_connect_result_handle(s2rhdr, data_buf, data_len);
                break;

            case S2R_DATA:
                ret = msg_data_handle(s2rhdr, data_buf, data_len);
                break;

            case REMOTE_CLOSED:
                ret = msg_remote_closed_handle(s2rhdr, data_buf, data_len);
                break;

            default:
                _LOG_ERROR("invalid subtype %d", s2rhdr->sub_type);
                return -1;
                break;
        }
    }
    else
    {
        _LOG_ERROR("recv pkt type %d invalid", pkthdr->pkt_type);
        return -1;
    }

    return ret;
}

int CSocksSrv::recv_handle(char *buf, int buf_len)
{
    int ret = 0;
    while (true)
    {
        if (buf_len > 0)
        {
            /*it has data not copy yet, copy it now*/
            int spare_len = sizeof(m_recv_buf) - m_recv_len;
            if (buf_len > spare_len)
            {
                memcpy(&m_recv_buf[m_recv_len], buf, spare_len);
                m_recv_len = sizeof(m_recv_buf);

                buf_len -= spare_len;
                buf += spare_len;
            }
            else
            {
                memcpy(&m_recv_buf[m_recv_len], buf, buf_len);
                m_recv_len += buf_len;

                buf_len = 0;
            }
        }

        if (m_recv_len < (int)(sizeof(PKT_HDR_T)))
        {
            _LOG_DEBUG("cur recv %d, wait for hdr", m_recv_len);
            return 0;
        }

        PKT_HDR_T *pkthdr = (PKT_HDR_T*)m_recv_buf;
        if (pkthdr->pkt_len > (sizeof(m_recv_buf) - sizeof(PKT_HDR_T)))
        {
            _LOG_ERROR("cur recv %d, but pktlen %u too long", m_recv_len, pkthdr->pkt_len);
            return -1;
        }
        
        if (m_recv_len < (int)(sizeof(PKT_HDR_T) + pkthdr->pkt_len))
        {
            _LOG_DEBUG("cur recv %d, need %d, wait for data", m_recv_len, (sizeof(PKT_HDR_T) + pkthdr->pkt_len));
            return 0;
        }

        int data_len = sizeof(PKT_HDR_T) + pkthdr->pkt_len;
        ret = pdu_handle(m_recv_buf, data_len);

        /*refill buffer*/        
        if (m_recv_len > data_len)
        {
            /*memcpy在64位ubuntu上有问题*/
            memmove(m_recv_buf, m_recv_buf + data_len, m_recv_len - data_len);
            m_recv_len -= data_len;
        }
        else
        {
            m_recv_len = 0;
            break;
        }
    }

    ret = ret;
    return 0;
}

void CSocksSrv::free_handle()
{
    g_SocksSrvMgr->del_socks_server(this);
    delete this;
}

void CSocksSrv::set_config(CServerCfg *srvCfg)
{
    m_srvCfg = *srvCfg;

    _LOG_INFO("set config for socks-server(%s:%u/%s:%u)", 
        m_ipstr, m_port, m_inner_ipstr, m_inner_port);
}

void CSocksSrv::add_client()
{
    m_client_cnt++;
}

void CSocksSrv::del_client()
{
    if (m_client_cnt == 0)
    {
        _LOG_ERROR("client cnt of socksserver %s alreay zero when dec", m_ipstr);
        return;
    }
    m_client_cnt--;
}

BOOL CSocksSrv::is_self(uint32_t pub_ipaddr, uint32_t inner_ipaddr)
{
    if( (pub_ipaddr == m_ipaddr) && (inner_ipaddr == m_inner_ipaddr))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL CSocksSrv::is_self(const char* pub_ipstr, const char* inner_ipstr)
{
    if( (0 == strncmp(m_ipstr, pub_ipstr, HOST_IP_LEN)
        && (0 == strncmp(m_inner_ipstr, inner_ipstr, HOST_IP_LEN))))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL CSocksSrv::is_self(uint32_t pub_ipaddr, char *username)
{
    if (pub_ipaddr != m_ipaddr)
    {
        return FALSE;
    }

    if (is_relay_need_auth() == FALSE)
    {
        return TRUE;
    }

    for (int ii = 0; ii < m_srvCfg.m_acct_cnt; ii++)
    {
        if ( 0 == strncmp(m_srvCfg.m_acct_infos[ii].username, username, MAX_USERNAME_LEN))
        {
            return TRUE;
        }
    }
    return FALSE;
}


void CSocksSrv::set_inner_info(uint32_t inner_ipaddr, uint16_t inner_port)
{
    m_inner_ipaddr = inner_ipaddr;
    m_inner_port = inner_port;

    struct sockaddr_storage ipaddr_s;
    memset((void*) &ipaddr_s, 0, sizeof(struct sockaddr_storage));
    ipaddr_s.ss_family = AF_INET;
    ((struct sockaddr_in *) &ipaddr_s)->sin_addr.s_addr = htonl(inner_ipaddr);
    if (NULL == util_ip_to_str(&ipaddr_s, m_inner_ipstr)) 
    {
        _LOG_ERROR("ip to str failed.");
    }
    m_inner_ipstr[HOST_IP_LEN] = 0;

    _LOG_INFO("set sockSrv(%s/%u) inner info: %s/%u", m_ipstr, m_port,
        m_inner_ipstr, m_inner_port);
}

void CSocksSrv::print_statistic(FILE *pFd)
{
    fprintf(pFd, "server-%s:%u inner %s:%u\n", m_ipstr, m_port,
        m_inner_ipstr, m_inner_port);

    fprintf(pFd, "\tsn:%s, acct cnt %d\n", m_srvCfg.m_sn, m_srvCfg.m_acct_cnt);
    for(int ii = 0; ii < m_srvCfg.m_acct_cnt; ii++)
    {
        fprintf(pFd, "\tusername:%s\t passwd:%s enabled:%d\n", 
            m_srvCfg.m_acct_infos[ii].username, 
            m_srvCfg.m_acct_infos[ii].passwd, 
            m_srvCfg.m_acct_infos[ii].enabled);
    }

    fprintf(pFd, "\tsend: client-close:%"PRIu64"\t connect-req:%"PRIu64"\t data:%"PRIu64"\n", 
        m_send_client_close_cnt, m_send_client_connect_cnt, m_send_data_cnt);
    fprintf(pFd, "\trecv: remote-close:%"PRIu64"\t connect-resp:%"PRIu64"\t data:%"PRIu64"\t srv-alive:%"PRIu64"\n", 
        m_recv_remote_close_cnt, m_recv_connect_result_cnt, m_recv_data_cnt, m_recv_srv_alive_cnt);

#if 0
    m_send_client_close_cnt = 0;
    m_send_client_connect_cnt = 0;
    m_send_data_cnt = 0;
    m_recv_remote_close_cnt = 0;
    m_recv_connect_result_cnt = 0;
    m_recv_data_cnt = 0;
    m_recv_srv_alive_cnt = 0;
#endif
}


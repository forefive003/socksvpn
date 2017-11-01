#include <netinet/tcp.h>
#include "commtype.h"
#include "logproc.h"
#include "CNetRecv.h"
#include "relay_pkt_def.h"
#include "netpool.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CClient.h"
#include "CRemote.h"
#include "CLocalServer.h"
#include "socketwrap.h"
#include "socks_server.h"
#include "CSocksMem.h"


void CLocalServer::set_self_pool_index(int index)
{
    m_self_pool_index = index;
}

int CLocalServer::connect_handle(BOOL result)
{
    if (!result)
    {
        return 0;
    }

#if 0
        int keepalive = 1; // 开启keepalive属性
        int keepidle = 10; // 如该连接在10秒内没有任何数据往来,则进行探测
        int keepinterval = 5; // 探测时发包的时间间隔为5 秒
        int keepcount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
        setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
        setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
        setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
        setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));
#endif
    if(0 != this->register_read())
    {
        return -1;
    }
    else
    {
        this->send_register();
    }
    return 0;
}

int CLocalServer::send_register()
{
    PKT_HDR_T pkthdr;
    PKT_SRV_REG_T reginfo;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&reginfo, 0, sizeof(PKT_SRV_REG_T));

    pkthdr.pkt_type = PKT_S2R_REG;
    pkthdr.pkt_len = sizeof(PKT_SRV_REG_T);
    PKT_HDR_HTON(&pkthdr);

    reginfo.local_ip = this->m_local_ipaddr;
    reginfo.local_port = this->m_local_port;
    reginfo.is_keepalive = 0;
    strncpy(reginfo.sn, g_server_sn, MAX_SN_LEN);
    PKT_SRV_REG_HTON(&reginfo);

    MUTEX_LOCK(m_local_srv_lock);
    if(0 != this->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    if(0 != this->send_data((char*)&reginfo, sizeof(PKT_SRV_REG_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    MUTEX_UNLOCK(m_local_srv_lock);

    _LOG_INFO("send register msg to relay(%s/%u/fd%d)", m_ipstr, m_port, m_fd);
    return 0;
}

int CLocalServer::send_keepalive()
{
    PKT_HDR_T pkthdr;
    PKT_SRV_REG_T reginfo;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&reginfo, 0, sizeof(PKT_SRV_REG_T));

    pkthdr.pkt_type = PKT_S2R_REG;
    pkthdr.pkt_len = sizeof(PKT_SRV_REG_T);
    PKT_HDR_HTON(&pkthdr);

    reginfo.local_ip = this->m_local_ipaddr;
    reginfo.local_port = this->m_local_port;
    reginfo.is_keepalive = 1;
    strncpy(reginfo.sn, g_server_sn, MAX_SN_LEN);
    PKT_SRV_REG_HTON(&reginfo);

    MUTEX_LOCK(m_local_srv_lock);
    if(0 != this->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    if(0 != this->send_data((char*)&reginfo, sizeof(PKT_SRV_REG_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }  
    MUTEX_UNLOCK(m_local_srv_lock);

    _LOG_DEBUG("remote(%s/%u/fd%d) send keepalive msg", m_ipstr, m_port, m_fd);
    return 0;
}


int CLocalServer::msg_request_handle(PKT_R2S_HDR_T *r2shdr, char *data_buf, int data_len)
{
    /**
    第一种
        05 01 00 03 13 77  65 62 2E 73 6F 75 72 63  65 66 6F 72 67 65 2E 6E  65 74 00 16
        1、05固定
        2、01说明是tcp
        3、00固定
        4、03说明后面跟着的是域名而不是ip地址，由socks5服务器进行dns解析
        5、13前面指明了是域名，那么0x13（19字节）是域名字符长度
        6、77 65 62 2E 73 6F 75 72 63 65 66 6F 72 67 65 2E 6E 65 74 就这19个是域名web.sourceforge.net的ascii。
        7、00 16端口，即为22端口。
    第二种
        05 01 00 01 CA 6C 16 05 00 50
        1、05固定
        2、01说明tcp
        3、00固定
        4、01说明是ip地址
        5、CA 6C 16 05就是202.108.22.5了，百度ip
        6、00 50端口，即为80端口
    */
    if (data_buf[0] != 0x05 || data_buf[2] != 0)
    {
        _LOG_ERROR("data buf invalid, 0-%u, 2-%u.", data_buf[0], data_buf[2]);
        return -1;
    }
    if (data_buf[1] != 0x01) 
    {
        _LOG_ERROR("data_buf[1] %d, only support CONNECT CMD now -_-", data_buf[1]);
        return -1;
    }

    if (data_len < 4)
    {
        _LOG_ERROR("data_len too less %d", data_len);
        return -1;
    }

    uint32_t remote_ipaddr = 0;
    char remote_domain[HOST_DOMAIN_LEN + 1] = {0};
    uint16_t remote_dstport = 0;

    switch (data_buf[3])
    { /* ATYP */
    case 0x01: /* IPv4 */
        if (data_len < 10)
        {
            _LOG_ERROR("data_len too less %d when parse ipv4", data_len);
            return -1;
        }

        memcpy(&remote_ipaddr, &data_buf[4], 4);
        remote_ipaddr = ntohl(remote_ipaddr);
        memcpy(&remote_dstport, &data_buf[8], 2);
        remote_dstport = ntohs(remote_dstport);
        break;
    case 0x03: /* FQDN */
        if (data_len < (5 + data_buf[4] + 2))
        {
            _LOG_ERROR("data_len too less %d when parse domain", data_len);
            return -1;
        }

        if (data_buf[4] > HOST_DOMAIN_LEN)
        {
            _LOG_ERROR("domain name too long %d", data_buf[4]);
            return -1;
        }
        memcpy(remote_domain, &data_buf[5], data_buf[4]);

        memcpy(&remote_dstport, &data_buf[5 + data_buf[4]], 2);
        remote_dstport = ntohs(remote_dstport);
        break;
    case 0x04: /* IPv6 */
        ///TODO:
        _LOG_WARN("IPV6 not support now");
        return -1;
        break;
    default:
        _LOG_ERROR("err ATYP: %x", data_buf[3]);
        return -1;
        break;
    }

    _LOG_INFO("recv new connect request from client");

    CConnection *pConn = new CConnection();
    g_ConnMgr->add_conn(pConn);
    
    CClient *pClient = new CClient(r2shdr->client_pub_ip, r2shdr->client_pub_port, -1, pConn, m_self_pool_index);
    g_total_client_cnt++;
    pClient->set_inner_info(r2shdr->client_inner_ip, r2shdr->client_inner_port);
    pConn->attach_client(pClient);
    if (0 != pClient->init())
    {
        pConn->detach_client();
        g_ConnMgr->del_conn(pConn);
        delete pClient;
        delete pConn;
        return -1;
    }

    /*create connection*/
    if (remote_domain[0] != 0)
    {
        uint64_t dns_req_time = util_get_cur_time();
        uint64_t dns_resp_time = 0;
        if( 0!= util_gethostbyname(remote_domain, &remote_ipaddr))
        {
            dns_resp_time = util_get_cur_time();
            _LOG_ERROR("get domain of server failed, domain %s, dns consume %ds", remote_domain,
                dns_resp_time - dns_req_time);
            /*if parse remote failed, shouldn't close this socks server*/
            pClient->send_connect_result(false);
            pClient->free();
            return 0;
        }
        dns_resp_time = util_get_cur_time();
        _LOG_INFO("getting remote hostname, domain %s, dns consume %ds", remote_domain,
            dns_resp_time - dns_req_time);
    }

    _LOG_INFO("get remote info, ipaddr 0x%x, port %u", remote_ipaddr, remote_dstport);
    CRemote *pRemote = new CRemote(remote_ipaddr, remote_dstport, -1, pConn);
//    pRemote->init_async_write_resource(socks_malloc, socks_free);

    /*init中注册连接事件,有可能会立即回调connect_handle, 需要保证之前已经初始化m_request_time等*/
    g_total_connect_req_cnt++;
    pRemote->m_request_time = util_get_cur_time();
    pConn->attach_remote(pRemote);

    if (0 != pRemote->init())
    {
        pConn->detach_remote();
        delete pRemote;
        pClient->send_connect_result(false);
        pClient->free();
        return 0;
    }

    return 0;
}

int CLocalServer::msg_client_close_handle(PKT_R2S_HDR_T *r2shdr, char *data_buf, int data_len)
{
    CConnection *pConn = (CConnection*)g_ConnMgr->get_conn_by_client(r2shdr->client_pub_ip, 
                                r2shdr->client_pub_port,
                                r2shdr->client_inner_ip,
                                r2shdr->client_inner_port);
    if (NULL == pConn)
    {
        char client_ipstr[HOST_IP_LEN] = { 0 };
        engine_ipv4_to_str(htonl(r2shdr->client_pub_ip), client_ipstr);

        _LOG_WARN("failed to find connection for client %s/%u, inner 0x%x/%u, when recv client close msg",  
                client_ipstr, r2shdr->client_pub_port,
                r2shdr->client_inner_ip, r2shdr->client_inner_port);
        return -1;
    }

    _LOG_INFO("recv client closed msg from (0x%x/%u/0x%x/%u)", 
        pConn->get_client_ipaddr(), pConn->get_client_port(),
        pConn->get_client_inner_ipaddr(), pConn->get_client_inner_port());

    MUTEX_LOCK(pConn->m_remote_lock);
    if (NULL != pConn->m_client)
    {
        pConn->m_client->free();
    }
    MUTEX_UNLOCK(pConn->m_remote_lock);

    pConn->dec_ref();
    return 0;
}

int CLocalServer::msg_data_handle(PKT_R2S_HDR_T *r2shdr, char *data_buf, int data_len)
{
    char client_ipstr[HOST_IP_LEN] = { 0 };
    engine_ipv4_to_str(htonl(r2shdr->client_pub_ip), client_ipstr);

    CConnection *pConn = (CConnection*)g_ConnMgr->get_conn_by_client(r2shdr->client_pub_ip, 
                                r2shdr->client_pub_port,
                                r2shdr->client_inner_ip,
                                r2shdr->client_inner_port);
    if (NULL == pConn)
    {
        _LOG_WARN("failed to find connection for client %s/%u, inner 0x%x/%u, when recv client data",  
                client_ipstr, r2shdr->client_pub_port,
                r2shdr->client_inner_ip, r2shdr->client_inner_port);
        return -1;
    }

    /*发送给远程*/
    if (0 != pConn->fwd_client_data_msg(data_buf, data_len))
    {
        _LOG_ERROR("send to remote failed for client %s/%u, inner 0x%x/%u",  
                client_ipstr, r2shdr->client_pub_port,
                r2shdr->client_inner_ip, r2shdr->client_inner_port);
        pConn->dec_ref();
        return -1;
    }

    pConn->dec_ref();
    return 0;
}

int CLocalServer::pdu_handle(char *pdu_buf, int pdu_len)
{
    int ret = 0;

    PKT_HDR_T *pkthdr = (PKT_HDR_T*)pdu_buf;

    PKT_R2S_HDR_T *r2shdr = (PKT_R2S_HDR_T*)(pdu_buf + sizeof(PKT_HDR_T));
    PKT_R2S_HDR_NTOH(r2shdr);

    char *data_buf = NULL;
    int data_len = 0;
    if (pkthdr->pkt_type != PKT_R2S)
    {
        _LOG_ERROR("recv pkt type %d invalid", pkthdr->pkt_type);
        return -1;
    }    

    data_buf = pdu_buf + sizeof(PKT_HDR_T) + sizeof(PKT_R2S_HDR_T);
    data_len = pkthdr->pkt_len - sizeof(PKT_R2S_HDR_T);
    switch(r2shdr->sub_type)
    {
        case CLIENT_CLOSED:
            ret = this->msg_client_close_handle(r2shdr, data_buf, data_len);
            break;

        case R2S_CLIENT_CONNECT:
            ret = this->msg_request_handle(r2shdr, data_buf, data_len);
            break;

        case R2S_DATA:
            ret = this->msg_data_handle(r2shdr, data_buf, data_len);
            break;

        default:
            _LOG_ERROR("invalid subtype %d", r2shdr->sub_type);
            return -1;
            break;
    }
    return ret;
}

int CLocalServer::recv_handle(char *buf, int buf_len)
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
        PKT_HDR_NTOH(pkthdr);
        
        if (pkthdr->pkt_len > (sizeof(m_recv_buf) - sizeof(PKT_HDR_T)))
        {
            _LOG_ERROR("cur recv %d, but pktlen %u too long", m_recv_len, pkthdr->pkt_len);
            return -1;
        }

        if (m_recv_len < (int)(sizeof(PKT_HDR_T) + pkthdr->pkt_len))
        {
            _LOG_DEBUG("cur recv %d, need %d, wait for data", m_recv_len, (sizeof(PKT_HDR_T) + pkthdr->pkt_len));
            PKT_HDR_HTON(pkthdr);
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

    /*if handle client message failed, shouldn't close self*/
    ret = ret;
    return 0;
    //return ret;
}

void CLocalServer::free_handle()
{
    ///TODO:
    //g_ConnMgr->free_all_conn();

    /*set to null, re init in timer*/
    g_localSrvPool->del_conn_obj(m_self_pool_index);
    delete this;
}

int CLocalServer::send_pre_handle()
{
    #if 0
    /*iterator all connection, and move their send node to self's*/
    CConnection *connObj = NULL;

    MUTEX_LOCK(g_ConnMgr->m_obj_lock);
    CONN_LIST_RItr itr = g_ConnMgr->m_conn_objs.rbegin();
    while (itr != g_ConnMgr->m_conn_objs.rend())
    {
        connObj = (CConnection*)*itr;
        itr++;

        MUTEX_LOCK(connObj->m_remote_lock);
        if (NULL != connObj->m_client)
        {
            if(connObj->m_client->m_send_q.node_cnt() > 0)
            {
                this->m_send_q.queue_cat(connObj->m_client->m_send_q);
            }
        }
        MUTEX_UNLOCK(connObj->m_remote_lock);        
    }
    MUTEX_UNLOCK(g_ConnMgr->m_obj_lock);
    #endif
    
    return 0;
}

int CLocalServer::send_post_handle()
{
    /*iterator all connection, register read evt if paused*/
    CConnection *connObj = NULL;

    MUTEX_LOCK(g_ConnMgr->m_obj_lock);
    CONN_LIST_RItr itr = g_ConnMgr->m_conn_objs.rbegin();
    while (itr != g_ConnMgr->m_conn_objs.rend())
    {
        connObj = (CConnection*)*itr;
        itr++;

        MUTEX_LOCK(connObj->m_event_lock);
        if (connObj->is_client_busy())
        {
            connObj->set_client_busy(false);
        }
        if (connObj->is_remote_pause_read())
        {
            /*register read event*/
            MUTEX_LOCK(connObj->m_remote_lock);
            if (connObj->m_remote != NULL)
                connObj->m_remote->resume_read();
            MUTEX_UNLOCK(connObj->m_remote_lock);

            connObj->set_remote_pause_read(false);
        }
        MUTEX_UNLOCK(connObj->m_event_lock);
    }
    MUTEX_UNLOCK(g_ConnMgr->m_obj_lock);
    return 0;
}

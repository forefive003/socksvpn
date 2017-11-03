
#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "CNetRecv.h"
#include "CClient.h"
#include "CRemote.h"
#include "CClientNet.h"
#include "CClientNetSet.h"
#include "CClientNetMgr.h"
#include "socks_relay.h"
#include "CSocksSrvMgr.h"

void CClientNet::set_self_pool_index(int index)
{
    m_self_pool_index = index;
}

int CClientNet::send_auth_result_msg(BOOL auth_ok)
{
    char resp_buf[2 + 4 + 4*32] = {0};

    resp_buf[0] = 0x01;
    if (!auth_ok)
    {
        resp_buf[1] = 0x01;
    }
    else
    {
        resp_buf[1] = 0x00;
    }

    int srv_ip_array[32] = {0};
    int srvs_cnt = g_SocksSrvMgr->get_running_socks_servers(srv_ip_array);
    if (srvs_cnt > 32) srvs_cnt = 32;

    int *ptmp = (int*)&resp_buf[2];
    *ptmp = htonl(srvs_cnt);
    ptmp++;

    for (int ii = 0; ii < srvs_cnt; ii++)
    {
        *ptmp = htonl(srv_ip_array[ii]);
        ptmp++;
    }

    int resp_len = 2 + 4 + srvs_cnt*4;
    
    PKT_HDR_T pkthdr;
    PKT_R2C_HDR_T r2chdr;

    memset(&pkthdr, 0, sizeof(PKT_HDR_T));
    memset(&r2chdr, 0, sizeof(PKT_R2C_HDR_T));

    pkthdr.pkt_type = PKT_R2C;
    pkthdr.pkt_len = sizeof(PKT_R2C_HDR_T) + resp_len;
    PKT_HDR_HTON(&pkthdr);

    r2chdr.server_ip = m_local_ipaddr;
    r2chdr.server_port = m_local_port;
    r2chdr.client_ip = m_inner_ipaddr;
    r2chdr.client_port = m_inner_port;
    r2chdr.sub_type = R2C_AUTH_RESULT;
    r2chdr.reserved = 0;

    PKT_R2C_HDR_HTON(&r2chdr);

    if(0 != this->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        return -1;
    }
    if(0 != this->send_data((char*)&r2chdr, sizeof(PKT_R2C_HDR_T)))
    {
        return -1;
    }
    if(0 != this->send_data(resp_buf, resp_len))
    {
        return -1;
    }

    _LOG_INFO("client(%s) send auth result msg", m_ipstr);
    return 0;
}

int CClientNet::parse_connect_req_msg(char *data_buf, int data_len)
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
        _LOG_ERROR("data buf invalid.");
        return -1;
    }
    if (data_buf[1] != 0x01) 
    {
        _LOG_ERROR("only support CONNECT CMD now -_-");
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
        _LOG_INFO("request connect remote ipaddr 0x%x/%u", remote_ipaddr, remote_dstport);
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
        _LOG_INFO("request connect remote domain %s, dport %u", remote_domain, remote_dstport);
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

    return 0;
}

/*
客户端发送01 06 6C 61 6F  74 73 65 06 36 36 36 38 38 38
1、01固定的
2、06这一个字节这是指明用户名长度，说明后面紧跟的6个字节就是用户名
3、6C 61 6F 74 73 65这就是那6个是用户名，是laotse的ascii
4、又一个06共1个字节指明密码长度，说明后面紧跟的6个字节就是密码
5、36 36 36 38 38 38就是这6个是密码，666888的ascii。
*/
int CClientNet::msg_auth_handle(PKT_C2R_HDR_T *c2rhdr, char *data_buf, int data_len)
{
    char username[MAX_USERNAME_LEN + 1] = {0};
    char passwd[MAX_PASSWD_LEN + 1] = {0};

    if (data_buf[0] != 0x01)
    {
        _LOG_ERROR("data buf invalid.");
        return -1;
    }

    m_recv_alive_cnt++;
    /*update time*/
    m_update_time = util_get_cur_time();

    /*copy username*/
    char *usr_pos = &data_buf[1];
    if (data_len < (1 + 1 + usr_pos[0]))
    {
        _LOG_ERROR("data_len too less for username, %d", data_len);
        return -1;
    }
    memcpy(username, &usr_pos[1], usr_pos[0]);

    /*copy passwd*/
    char *pass_pos = &data_buf[2 + usr_pos[0]];
    if (data_len < (1 + 1 + usr_pos[0] + 1 + pass_pos[0]))
    {
        _LOG_ERROR("data_len too less for passwd, len %d, user len %u, pass len %d", data_len, usr_pos[0], pass_pos[0]);
        return -1;
    }
    memcpy(passwd, &pass_pos[1], pass_pos[0]);

    CSocksNetSet *socksSetNet = NULL;

    g_SocksSrvMgr->lock();
    socksSetNet = g_SocksSrvMgr->get_socks_server_by_auth(c2rhdr->server_ip, username, passwd);
    if (NULL == socksSetNet)
    {
        g_SocksSrvMgr->unlock();

        char ipstr[64] = { 0 };
        engine_ipv4_to_str(htonl(c2rhdr->server_ip), ipstr);
        _LOG_ERROR("no server with user %s has registered on pub ipaddr %s", username, ipstr);

        /*response to client, auth failed*/
        this->send_auth_result_msg(FALSE);
        m_is_authed = FALSE;
        return -1;
    }

    /*set pub ip and private ip for the socks server of this client channel*/
    m_srv_pub_ipaddr = socksSetNet->m_ipaddr;
    m_srv_private_ipaddr = socksSetNet->m_private_ipaddr;
    g_SocksSrvMgr->unlock();

    /*response to client, auth success*/
    this->send_auth_result_msg(TRUE);
    this->m_is_authed = TRUE;
    /*set inner info*/
    this->set_inner_info(c2rhdr->client_ip, c2rhdr->client_port);

    /*add to set*/
    g_ClientNetMgr->add_netobj(m_self_pool_index, m_ipaddr, m_inner_ipaddr);

    _LOG_INFO("client(%s/%s) auth ok.", m_ipstr, m_inner_ipstr);
    return 0;
}

int CClientNet::msg_connect_handle(PKT_C2R_HDR_T *c2rhdr, char *data_buf, int data_len)
{
    if (!m_is_authed)
    {
        _LOG_WARN("client(%s) isn't in authed when recv connect msg.", m_ipstr);
        return -1;
    }

    _LOG_INFO("recv connect request from client(0x%x/%u/0x%x/%u/fd%d)", 
        m_ipaddr, m_port,
        c2rhdr->client_ip, c2rhdr->client_port, m_fd);
    
    this->m_recv_connect_request_cnt++;

    CConnection *pConn = new CConnection();
    g_ConnMgr->add_conn(pConn);

    /*create client, clientip is private addr*/
    CClient *pClient = new CClient(m_ipaddr, m_port, -1, pConn, m_self_pool_index);
    g_total_client_cnt++;
    pClient->set_inner_info(c2rhdr->client_ip, c2rhdr->client_port);
    pConn->attach_client(pClient);

    if (0 != pClient->init())
    {
        pConn->detach_client();
        g_ConnMgr->del_conn(pConn);
        delete pClient;
        delete pConn;
        return -1;
    }    

    /*create remote, serverip is public addr*/
    int remotePoolIndex = g_SocksSrvMgr->get_active_socks_server(this->m_srv_pub_ipaddr, this->m_srv_private_ipaddr);
    if (-1 == remotePoolIndex)
    {
        _LOG_WARN("fail to get active server conn obj for pub 0x%x, pri 0x%x", 
            this->m_srv_pub_ipaddr, this->m_srv_private_ipaddr);
        pConn->detach_client();
        g_ConnMgr->del_conn(pConn);
        delete pClient;
        delete pConn;
        return -1;
    }

    CRemote *pRemote = new CRemote(c2rhdr->server_ip, c2rhdr->server_port, -1, pConn, remotePoolIndex);    
    pRemote->set_username(m_username);
    pConn->attach_remote(pRemote);

    if (0 != pRemote->init())
    {
        pConn->detach_remote();
        delete pRemote;
        pClient->free();
        return -1;
    }
    
    g_total_remote_cnt++;
    //parse_connect_req_msg(data_buf, data_len);    
    return pConn->fwd_client_connect_msg(data_buf, data_len);
}

int CClientNet::msg_client_closed_handle(PKT_C2R_HDR_T *c2rhdr, char *data_buf, int data_len)
{
    if (!m_is_authed)
    {
        _LOG_WARN("client(%s) isn't in authed when recv client closed msg.", m_ipstr);
        return -1;
    }

    _LOG_INFO("recv client closed msg from (0x%x/%u/0x%x/%u/fd%d)",
        m_ipaddr, m_port, 
        c2rhdr->client_ip, c2rhdr->client_port, m_fd);

    this->m_recv_client_close_cnt++;
    CConnection *pConn = (CConnection*)g_ConnMgr->get_conn_by_client(m_ipaddr, m_port, 
                                        c2rhdr->client_ip, c2rhdr->client_port);
    if (NULL == pConn)
    {
        _LOG_WARN("failed to find connection for client 0x%x/%u/0x%x/%u, when recv client close", 
            m_ipaddr, m_port,
            c2rhdr->client_ip, c2rhdr->client_port);
        return -1;
    }  

    MUTEX_LOCK(pConn->m_remote_lock);
    if (pConn->m_client != NULL)
    {
            pConn->m_client->free();
    }
    MUTEX_UNLOCK(pConn->m_remote_lock);

    pConn->dec_ref();
    return 0;
}

int CClientNet::msg_data_handle(PKT_C2R_HDR_T *c2rhdr, char *data_buf, int data_len)
{
    if (!m_is_authed)
    {
        _LOG_WARN("client(%s) isn't in authed when recv data msg.", m_ipstr);
        return -1;
    }
    
    this->m_recv_data_cnt++;

    CConnection *pConn = (CConnection*)g_ConnMgr->get_conn_by_client(m_ipaddr, m_port, 
                                        c2rhdr->client_ip, c2rhdr->client_port);
    if (NULL == pConn)
    {
        _LOG_WARN("failed to find connection for client 0x%x/%u/0x%x/%u, when recv client data", 
            m_ipaddr, m_port,
            c2rhdr->client_ip, c2rhdr->client_port);
        return -1;
    }  

    int ret = pConn->fwd_client_data_msg(data_buf, data_len);
    pConn->dec_ref();
    return ret;
}

int CClientNet::pdu_handle(char *pdu_buf, int pdu_len)
{
    int ret = 0;
    PKT_HDR_T *pkthdr = (PKT_HDR_T*)pdu_buf;

    PKT_C2R_HDR_T *c2rhdr = (PKT_C2R_HDR_T*)(pdu_buf + sizeof(PKT_HDR_T));
    PKT_C2R_HDR_NTOH(c2rhdr);
    
    char *data_buf = NULL;
    int data_len = 0;
    if (pkthdr->pkt_type != PKT_C2R)
    {
        _LOG_ERROR("client(%s/%u/fd%d) recv pkt type %d invalid",
            m_ipstr, m_port, m_fd, pkthdr->pkt_type);
        return -1;
    }

    data_buf = pdu_buf + sizeof(PKT_HDR_T) + sizeof(PKT_C2R_HDR_T);
    data_len = pkthdr->pkt_len - sizeof(PKT_C2R_HDR_T);

    switch(c2rhdr->sub_type)
    {
        case C2R_AUTH:
            ret = msg_auth_handle(c2rhdr, data_buf, data_len);
            break;

        case C2R_CONNECT:
            ret = msg_connect_handle(c2rhdr, data_buf, data_len);
            break;

        case C2R_DATA:
            ret = msg_data_handle(c2rhdr, data_buf, data_len);
            break;

        case CLIENT_CLOSED:
            ret = msg_client_closed_handle(c2rhdr, data_buf, data_len);
            break;

        default:
            _LOG_ERROR("invalid subtype %d", c2rhdr->sub_type);
            return -1;
            break;            
    }
    return ret;
}

int CClientNet::recv_handle(char *buf, int buf_len)
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

        if (m_recv_len < (int)(sizeof(PKT_HDR_T) + sizeof(PKT_C2R_HDR_T)))
        {
            _LOG_DEBUG("cur recv %d, wait for hdr", m_recv_len);
            return 0;
        }

        PKT_HDR_T *pkthdr = (PKT_HDR_T*)m_recv_buf;
        PKT_HDR_NTOH(pkthdr);
        if (pkthdr->pkt_len > (sizeof(m_recv_buf) - sizeof(PKT_HDR_T)))
        {
            _LOG_ERROR("cur recv %d, but pktlen %u too long in client", m_recv_len, pkthdr->pkt_len);
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
    
    ret = ret;
    return 0;
}

void CClientNet::free_handle()
{
    ///TODO:
    //g_ConnMgr->free_all_conn();
    
    /*从管理中删除*/
    g_ClientNetMgr->del_netobj(m_self_pool_index, m_ipaddr, m_inner_ipstr);

    /*从连接池中删除*/
    g_clientNetPool->del_conn_obj(m_self_pool_index);
    delete this;
}

BOOL CClientNet::is_self(uint32_t pub_ipaddr, uint16_t pub_port)
{
    if( (pub_ipaddr == m_ipaddr) && (pub_port == m_port))
    {
        return TRUE;
    }

    return FALSE;
}


void CClientNet::set_inner_info(uint32_t inner_ipaddr, uint16_t inner_port)
{
    m_inner_ipaddr = inner_ipaddr;
    m_inner_port = inner_port;

    engine_ipv4_to_str(htonl(inner_ipaddr), m_inner_ipstr);
    m_inner_ipstr[HOST_IP_LEN] = 0;

    _LOG_INFO("set clientSrv(%s/%u) inner info: %s/%u", m_ipstr, m_port,
        m_inner_ipstr, m_inner_port);
}

void CClientNet::set_user_passwd(char *username, char *passwd)
{
    memset(m_username, 0, sizeof(m_username));
    strncpy(m_username, username, MAX_USERNAME_LEN);

    memset(m_passwd, 0, sizeof(m_passwd));
    strncpy(m_passwd, passwd, MAX_PASSWD_LEN);
}

void CClientNet::print_statistic(FILE *pFd)
{
    char dateformat[64] = {'\0'};
    struct tm uptimeTm;
    #ifdef _WIN32
    localtime_s(&uptimeTm, (time_t*)&this->m_update_time);
    #else
    localtime_r((time_t*)&this->m_update_time, &uptimeTm);
    #endif

    sprintf(dateformat, "%04d-%02d-%02d %02d:%02d:%02d", 
        uptimeTm.tm_year + 1900, 
        uptimeTm.tm_mon + 1, 
        uptimeTm.tm_mday, 
        uptimeTm.tm_hour, 
        uptimeTm.tm_min, 
        uptimeTm.tm_sec);

    fprintf(pFd, "client-%s:%u inner %s:%u, update-time %s\n", m_ipstr, m_port,
        m_inner_ipstr, m_inner_port, dateformat);
    fprintf(pFd, "\tsend: remote-close:%"PRIu64"\t connect-resp:%"PRIu64"\t data:%"PRIu64"\n", 
        m_send_remote_close_cnt, m_send_connect_result_cnt, m_send_data_cnt);
    fprintf(pFd, "\trecv: client-close:%"PRIu64"\t connect-req:%"PRIu64"\t data:%"PRIu64"\t alive:%"PRIu64"\n", 
        m_recv_client_close_cnt, m_recv_connect_request_cnt, m_recv_data_cnt, m_recv_alive_cnt);

#if 0
    m_send_remote_close_cnt = 0;
    m_send_connect_result_cnt = 0;
    m_send_data_cnt = 0;
    m_recv_client_close_cnt = 0;
    m_recv_connect_request_cnt = 0;
    m_recv_data_cnt = 0;
#endif
}

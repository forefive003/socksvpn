#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "CNetRecv.h"
#include "CNetAccept.h"
#include "CClient.h"
#include "CRemote.h"
#include "CLocalServer.h"
#include "socks_server.h"
#include "CSocksMem.h"
#include "socketwrap.h"


const char* g_cli_status_desc[] =
{
    "INVALID",
    "INIT",
    "AUTHING",
    "CONNECTING",
    "CONNECTED",
};


int CClient::send_connect_result(BOOL result)
{
    if (false == result)
    {
        /*not need send response when connect failed*/
        return 0;
    }

    if (m_sock_ver == SOCKS_5)
    {
        char resp_buf[10] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        if(0 != this->send_data(resp_buf, sizeof(resp_buf)))
        {
            _LOG_ERROR("send response failed");
            return -1;
        }
    }
    else
    {        
        /*回应, 类似 04 5A 00 50 CA 6C 16 05*/
        char resp_buf[8] = {0};

        uint16_t *remote_port = (uint16_t*)&resp_buf[2];
        uint32_t *remote_ipaddr = (uint32_t*)&resp_buf[4];
        resp_buf[0] = 0x00;
        resp_buf[1] = 0x5A;
        *remote_port = htons(m_remote_dstport);
        *remote_ipaddr = htonl(m_remote_ipaddr);
        if (0 != this->send_data(resp_buf, 8))
        {
            _LOG_ERROR("send response failed");
            return -1;
        }
    }

    _LOG_INFO("client(%s/%u/%s/%u/fd%d) send connect result to client", m_ipstr, m_port, 
        m_inner_ipstr, m_inner_port, m_fd);

    this->set_client_status(CLI_CONNECTED);     
    return 0;
}

int CClient::try_connect_remote()
{
    CConnection *pConn = (CConnection *)this->m_owner_conn;

    CRemote *pRemote = new CRemote(m_remote_ipaddr, m_remote_dstport, -1, pConn);

    /*init中注册连接事件,有可能会立即回调connect_handle, 需要保证之前已经初始化m_request_time等*/
    g_total_connect_req_cnt++;
    pRemote->m_request_time = util_get_cur_time();
    pConn->attach_remote(pRemote);

    if (0 != pRemote->init())
    {
        pConn->detach_remote();
        delete pRemote;
        return -1;
    }

    return 0;
}

/*
SOCKS4: 支持ID认证, 不支持远端域名解析
04 01 00 50 CA 6C 16 05  41 64 6D 69 6E 69 73 74 72 61 74 6F 72 00
1、其中开头04 01是固定的
2、00 50是要连接远程站点的端口号，这里是80端口，十六进制就是00 50
3、CA 6C 16 05就是远程IP地址, 202.108.22.5的十六进制
4、41 64 6D 69 6E 69 73 74 72 61 74 6F 72是Administrator的acsii码
5、最后一个00也是固定的
这就是socks4的固定格式04 01+端口2字节+ip4字节+id+00，其中的id，这里是Administrator是可有也可以没有，因为我现在win7用的账户是Administrator所以ie就把账号名给发过去了，我试了一下最新的火狐4.0，他发送MOZ，有的就直接省略掉了
04 01 00 50 CA 6C 16 05 00 所以发成这样也行，随便写什么都行，只要最后一个是0中间没有0就行了。

如果代理服务器允许代理，比如验证一下那个id，比如你想只允许id是administrator后，允许代理了，就发回8个字节
04 5A 00 50 CA 6C 16 05
也就是01变成了5A, 也可以换成其他的值

SOCKS4A: 支持ID认证, 支持远端域名解析
04 01 00 16 00 00 00 01  6C 61 6F 74 73 65 00 65 62 2E 73 6F 75 72 63  65 66 6F 72 67 65 2E 6E  65 74 00
1、04 01是固定的
2、00 16是端口22的十六进制
3、00 00 00 01是原本ip地址的地方，这里前3个字节必须为0，最后一个必须不能为0，一般都是1
4、6C 61 6F 74 73 65是laotse的ascii，我设的flashfxp代理的命名，可以没有，和sock4一样
5、00 固定的，和sock4一样，如果第四步的id没有，这个00也不能省略
6、65 62 2E 73 6F 75 72 63 65 66 6F 72 67 65 2E 6E 65 74 就是域名web.sourceforge.net的ascii了
7、00 固定的。
这样就可以看出，就是把sock4的ip那地方换成00 00 00 xx这样的假ip，然后再sock4后面多了域名+00
*/
int CClient::socks4_handshake_request_handle(char *buf, int buf_len)
{
    int spare_len = sizeof(m_recv_buf) - m_recv_len;
    if (buf_len > spare_len)
    {
        _LOG_ERROR("handshake request buf too long(now %d)", buf_len);
        return -1;
    }

    memcpy(&m_recv_buf[m_recv_len], buf, buf_len);
    m_recv_len += buf_len;

    if (m_recv_len < 9 && m_recv_buf[m_recv_len-1] != 0x0)
    {
        _LOG_WARN("handshake buf not recv completed, wait");
        return 0;
    }

    if (m_recv_buf[4] == 0 && m_recv_buf[5] == 0 && m_recv_buf[6] == 0)
    {
        /*解析域名, 从第10个字节开始*/
        if (m_recv_len < 10)
        {
            _LOG_WARN("handshake buf not recv completed, wait domain name");
            return 0;
        }

        char remote_domain[HOST_DOMAIN_LEN + 1] = {0};
        memcpy(remote_domain, &m_recv_buf[9], m_recv_len-9);

        _LOG_INFO("getting remote hostname in socks4a, domain %s", remote_domain);
        if( 0!= util_gethostbyname(remote_domain, &m_remote_ipaddr))
        {
            _LOG_ERROR("get domain of server failed, domain %s", remote_domain);
            return -1;
        }

        memcpy(&m_remote_dstport, &m_recv_buf[2], 2);
        m_remote_dstport = ntohs(m_remote_dstport);
        _LOG_INFO("get remote info in socks4a, ipaddr 0x%x, port %u, domain %s", 
            m_remote_ipaddr, m_remote_dstport, remote_domain);
    }
    else
    {           
        memcpy(&m_remote_ipaddr, &m_recv_buf[4], 4);
        m_remote_ipaddr = ntohl(m_remote_ipaddr);
        memcpy(&m_remote_dstport, &m_recv_buf[2], 2);
        m_remote_dstport = ntohs(m_remote_dstport);
        _LOG_INFO("get remote info in socks4, ipaddr 0x%x, port %u", m_remote_ipaddr, m_remote_dstport);
    }
    
    /*对于socks4, 后面等连接上远程服务器后直接进行数据交互*/
    return this->try_connect_remote();
}

/*
SOCKS5: 支持用户名/密码认证, 支持远端域名解析, 支持UDP代理
05 01 00 共3字节，这种是要求匿名代理
05 01 02 共3字节，这种是要求以用户名密码方式验证代理
05 02 00 02 共4字节，这种是要求以匿名或者用户名密码方式代理
*/
int CClient::socks5_handshake_request_handle(char *buf, int buf_len)
{
    if (buf_len < 3)
    {
        _LOG_ERROR("buf 0x%02x invalid, len %d, handshake failed.", buf[0], buf_len);
        return -1;
    }

    int len = buf[1];
    if ((len + 1) > buf_len)
    {
        _LOG_ERROR("buf too less, handshake failed");
        return -1;
    }

    for (int i = 0; i < len; i++)
    {
        if (buf[i+2] == 0)
        {
            _LOG_INFO("client want anonimous proxy");

            /*回应, 类似 05 00*/
            char resp_buf[2] = {0};
            resp_buf[0] = 0x05;
            resp_buf[1] = 0x00;
            if (0 != this->send_data(resp_buf, 2))
            {
                _LOG_ERROR("send response failed");
                return -1;
            }

            this->set_client_status(CLI_CONNECTING);
            return 0;
        }
        else if (buf[i+2] == 2)
        {
            _LOG_WARN("client want user-password proxy");
            /*回应, 类似 05 02*/
            char resp_buf[2] = {0};
            resp_buf[0] = 0x05;
            resp_buf[1] = 0x02;
            if (0 != this->send_data(resp_buf, 2))
            {
                _LOG_ERROR("send response failed");
                return -1;
            }

            this->set_client_status(CLI_AUTHING);
            return 0;
        }
        else
        {
            _LOG_WARN("client want proxy method %02x, ignore it", buf[i+2]);
            return -1;
        }
    }

    return -1;
}

/*
客户端发送01 06 6C 61 6F  74 73 65 06 36 36 36 38 38 38
1、01固定的
2、06这一个字节这是指明用户名长度，说明后面紧跟的6个字节就是用户名
3、6C 61 6F 74 73 65这就是那6个是用户名，是laotse的ascii
4、又一个06共1个字节指明密码长度，说明后面紧跟的6个字节就是密码
5、36 36 36 38 38 38就是这6个是密码，666888的ascii。
*/
int CClient::socks5_auth_request_handle(char *buf, int buf_len)
{
    int spare_len = sizeof(m_recv_buf) - m_recv_len;
    if (buf_len > spare_len)
    {
        _LOG_ERROR("auth request buf too long(now %d)", buf_len);
        return -1;
    }

    memcpy(&m_recv_buf[m_recv_len], buf, buf_len);
    m_recv_len += buf_len;

    char username[MAX_USERNAME_LEN + 1] = {0};
    char passwd[MAX_PASSWD_LEN + 1] = {0};

    if (m_recv_buf[0] != 0x01)
    {
        _LOG_ERROR("data buf invalid.");
        return -1;
    }

    /*copy username*/
    char *usr_pos = &m_recv_buf[1];
    if (m_recv_len < (1 + 1 + usr_pos[0]))
    {
        _LOG_WARN("data_len too less for username, %d", m_recv_len);
        return 0;
    }
    memcpy(username, &usr_pos[1], usr_pos[0]);

    /*copy passwd*/
    char *pass_pos = &m_recv_buf[2 + usr_pos[0]];
    if (m_recv_len < (1 + 1 + usr_pos[0] + 1 + pass_pos[0]))
    {
        _LOG_WARN("data_len too less for passwd, len %d, user len %u, pass len %d", m_recv_len, usr_pos[0], pass_pos[0]);
        return 0;
    }
    memcpy(passwd, &pass_pos[1], pass_pos[0]);

    _LOG_INFO("client(%s) auth ok, username %s, passwd %s.", m_ipstr, username, passwd);

    char resp_buf[2] = {0x01, 0x00};
    if (0 != this->send_data(resp_buf, 2))
    {
        _LOG_ERROR("send response failed");
        return -1;
    }

    this->set_client_status(CLI_CONNECTING);
    return 0;
}

int CClient::socks5_connect_request_handle(char *buf, int buf_len)
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
    int spare_len = sizeof(m_recv_buf) - m_recv_len;
    if (buf_len > spare_len)
    {
        _LOG_ERROR("connect request buf too long(now %d)", buf_len);
        return -1;
    }

    memcpy(&m_recv_buf[m_recv_len], buf, buf_len);
    m_recv_len += buf_len;

    if (m_recv_buf[0] != 0x05 || m_recv_buf[2] != 0)
    {
        _LOG_ERROR("data buf invalid, 0-%u, 2-%u.", m_recv_buf[0], m_recv_buf[2]);
        return -1;
    }
    if (m_recv_buf[1] != 0x01) 
    {
        _LOG_ERROR("data_buf[1] %d, only support CONNECT CMD now -_-", m_recv_buf[1]);
        return -1;
    }

    if (m_recv_len < 4)
    {
        _LOG_ERROR("data_len too less %d", m_recv_len);
        return -1;
    }

    char remote_domain[HOST_DOMAIN_LEN + 1] = {0};

    switch (m_recv_buf[3])
    { /* ATYP */
    case 0x01: /* IPv4 */
        if (m_recv_len < 10)
        {
            _LOG_ERROR("data_len too less %d when parse ipv4", m_recv_len);
            return -1;
        }

        memcpy(&m_remote_ipaddr, &m_recv_buf[4], 4);
        m_remote_ipaddr = ntohl(m_remote_ipaddr);
        memcpy(&m_remote_dstport, &m_recv_buf[8], 2);
        m_remote_dstport = ntohs(m_remote_dstport);
        break;
    case 0x03: /* FQDN */
        if (m_recv_len < (5 + m_recv_buf[4] + 2))
        {
            _LOG_ERROR("data_len too less %d when parse domain", m_recv_len);
            return -1;
        }

        if (m_recv_buf[4] > HOST_DOMAIN_LEN)
        {
            _LOG_ERROR("domain name too long %d", m_recv_buf[4]);
            return -1;
        }
        memcpy(remote_domain, &m_recv_buf[5], m_recv_buf[4]);

        memcpy(&m_remote_dstport, &m_recv_buf[5 + m_recv_buf[4]], 2);
        m_remote_dstport = ntohs(m_remote_dstport);
        break;
    case 0x04: /* IPv6 */
        ///TODO:
        _LOG_WARN("IPV6 not support now");
        return -1;
        break;
    default:
        _LOG_ERROR("err ATYP: %x", m_recv_buf[3]);
        return -1;
        break;
    }

    /*create connection*/
    if (remote_domain[0] != 0)
    {
        _LOG_INFO("getting remote hostname, domain %s", remote_domain);
        if( 0!= util_gethostbyname(remote_domain, &m_remote_ipaddr))
        {
            _LOG_ERROR("get domain of server failed, domain %s", remote_domain);
            return -1;
        }
    }

    _LOG_INFO("get remote info, ipaddr 0x%x, port %u", m_remote_ipaddr, m_remote_dstport);

    return this->try_connect_remote();
}

int CClient::recv_handle(char *buf, int buf_len)
{
    int ret = 0;
    CConnection *pConn = (CConnection *)this->m_owner_conn;

    _LOG_DEBUG("recv from client(%s/%u/fd%d)", m_ipstr, m_port, m_fd);

    switch(this->m_status)
    {
    case CLI_INIT:
        if (m_sock_ver == SOCKS_INVALID)
        {
            if (buf[0] == 0x04)
            {
                m_sock_ver = SOCKS_4;
            }
            else if (buf[0] == 0x05)
            {
                m_sock_ver = SOCKS_5;
            }
            else
            {
                _LOG_ERROR("client(%s/%u/%s/%u/fd%d), invalid socks version(%d)", m_ipstr, m_port, 
                    m_inner_ipstr, m_inner_port, m_fd, buf[0]);
                return -1;
            }
        }

        if (m_sock_ver == SOCKS_4)
        {
            if (0 != this->socks4_handshake_request_handle(buf, buf_len))
            {
                _LOG_ERROR("client(%s/%u/%s/%u/fd%d), handle socks4 failed", m_ipstr, m_port, 
                    m_inner_ipstr, m_inner_port, m_fd);
                return -1;
            }
        }
        else if (m_sock_ver == SOCKS_5)
        {
            if (0 != this->socks5_handshake_request_handle(buf, buf_len))
            {
                _LOG_ERROR("client(%s/%u/%s/%u/fd%d), handle socks5 failed", m_ipstr, m_port, 
                    m_inner_ipstr, m_inner_port, m_fd);
                return -1;
            }
        }
        break;

    case CLI_AUTHING:
        if (0 != this->socks5_auth_request_handle(buf, buf_len))
        {
            return -1;
        }
        break;

    case CLI_CONNECTING:
        if (0 != this->socks5_connect_request_handle(buf, buf_len))
        {
            return -1;
        }       
        break;

    case CLI_CONNECTED:
        /*发送给远程*/
        ret = pConn->fwd_client_data_msg(buf, buf_len);
        if (0 != ret)
        {
            _LOG_ERROR("client(%s/%u/%s/%u/fd%d), send to remote failed", m_ipstr, m_port, 
                m_inner_ipstr, m_inner_port, m_fd);
            return -1;
        }
        break;

    default:
        /// can't be here
        _LOG_ERROR("client(%s/%u/%s/%u/fd%d) status %d, can't be here", m_ipstr, m_port, 
                m_inner_ipstr, m_inner_port, m_fd, m_status);
        return -1;
        break;
    }

    return 0;
}

void CClient::set_client_status(CLI_STATUS_E status)
{
    if (status == m_status)
    {
        _LOG_ERROR("client(%s/%u/%s/%u/fd%d) status already %s", m_ipstr, m_port, 
            m_inner_ipstr, m_inner_port,
            m_fd, g_cli_status_desc[m_status]);
        return;
    }

    memset(m_recv_buf, 0, sizeof(m_recv_buf));
    m_recv_len = 0;

    m_status = status;
    _LOG_INFO("client(%s/%u/%s/%u/fd%d) set to status %s", m_ipstr, m_port, 
        m_inner_ipstr, m_inner_port,
        m_fd, g_cli_status_desc[m_status]);
    return;
}

int CClient::get_client_status()
{
    return m_status;
}

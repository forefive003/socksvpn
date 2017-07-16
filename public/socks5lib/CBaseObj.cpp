#include "commtype.h"
#include "logproc.h"
#include "CBaseObj.h"
#include "CBaseConn.h"

CBaseObj::CBaseObj(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *conn):CNetRecv(ipaddr,port,fd)
{
    m_owner_conn = conn;
    assert(m_owner_conn);
}
CBaseObj::~CBaseObj()
{
}

int CBaseObj::send_data_msg(char *buf, int buf_len)
{
    return this->send_data(buf, buf_len);
}
BOOL CBaseObj::is_self(uint32_t ipaddr, uint16_t port)
{
    return (m_ipaddr == ipaddr && m_port == port);
}
uint32_t CBaseObj::get_ipaddr()
{
    return m_ipaddr;
}
char* CBaseObj::get_ipstr()
{
    return m_ipstr;
}
uint16_t CBaseObj::get_port()
{
    return m_port;
}
int CBaseObj::get_fd()
{
    return m_fd;
}

CBaseClient::CBaseClient(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *conn) : CBaseObj(ipaddr,port,fd, conn)
{
    this->set_thrd_index(0);

    m_inner_ipaddr = 0;
    memset(m_inner_ipstr, 0, sizeof(m_inner_ipstr));
    m_inner_port = 0;
    _LOG_INFO("new client(%s/%u/fd %d)", get_ipstr(), get_port(), get_fd());
}
CBaseClient::~CBaseClient()
{
    _LOG_INFO("destroy client(%s/%u/%s/%u/fd %d)", get_ipstr(), get_port(),
        m_inner_ipstr, m_inner_port, get_fd());
}
void CBaseClient::free_handle()
{
    /*最后的delete this会设置m_owner_conn为NULL,先局部变量缓存*/
    CBaseConnection *owner_conn = m_owner_conn;

    owner_conn->inc_ref();

    /*先发送报文*/
    owner_conn->notify_client_close();
    /*必须先detach*/
    owner_conn->detach_client();
    /*在释放client*/
    owner_conn->free_remote();
    delete this;
    
    owner_conn->dec_ref();    
}

void CBaseClient::set_inner_info(uint32_t inner_ipaddr, uint16_t inner_port)
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

    _LOG_INFO("set client(%s/%u) inner info: %s/%u", get_ipstr(), get_port(),
        m_inner_ipstr, m_inner_port);
}
uint32_t CBaseClient::get_inner_ipaddr()
{
    return m_inner_ipaddr;
}
uint16_t CBaseClient::get_inner_port()
{
    return m_inner_port;
}
BOOL CBaseClient::is_self(uint32_t ipaddr, uint16_t port, uint32_t inner_ipaddr, uint16_t inner_port)
{
    return (m_ipaddr == ipaddr && m_port == port
        && m_inner_ipaddr == inner_ipaddr && m_inner_port == inner_port);
}

CBaseRemote::CBaseRemote(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *conn) : CBaseObj(ipaddr,port,fd, conn)
{
    this->set_thrd_index(1);
    
    _LOG_INFO("new remote(%s/%u/fd %d), client 0x%x/%u//0x%x/%u/fd %d", 
        get_ipstr(), get_port(), get_fd(),
        m_owner_conn->get_client_ipaddr(), m_owner_conn->get_client_port(),
        m_owner_conn->get_client_inner_ipaddr(), m_owner_conn->get_client_inner_port(),
        m_owner_conn->get_client_fd());
}
CBaseRemote::~CBaseRemote()
{
    _LOG_INFO("destroy remote(%s/%u/fd %d), client 0x%x/%u//0x%x/%u/fd %d", 
        get_ipstr(), get_port(), get_fd(),
        m_owner_conn->get_client_ipaddr(), m_owner_conn->get_client_port(),
        m_owner_conn->get_client_inner_ipaddr(), m_owner_conn->get_client_inner_port(),
        m_owner_conn->get_client_fd());
}
void CBaseRemote::free_handle()
{
    /*最后的delete this会设置m_owner_conn为NULL,先局部变量缓存*/
    CBaseConnection *owner_conn = m_owner_conn;

    owner_conn->inc_ref();

    /*先发送报文*/
    owner_conn->notify_remote_close();
    /*必须先detach*/
    owner_conn->detach_remote();
    /*在释放client*/
    owner_conn->free_client();
    delete this;

    owner_conn->dec_ref();    
}

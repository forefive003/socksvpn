#include "commtype.h"
#include "logproc.h"
#include "CBaseObj.h"
#include "CBaseConn.h"
#include "CConMgr.h"


uint64_t g_total_data_req_cnt = 0;
uint64_t g_total_data_req_byte = 0;
uint64_t g_total_data_resp_cnt = 0;
uint64_t g_total_data_resp_byte = 0;

CBaseConnection::CBaseConnection()
{
    m_client = NULL;
    m_remote = NULL;

    m_send_client_data_cnt = 0;
    m_send_remote_data_cnt = 0;

    m_send_client_bytes = 0;
    m_send_remote_bytes = 0;

    m_is_client_busy = false;
    m_is_remote_busy = false;

    m_setup_time = util_get_cur_time();
        
    MUTEX_SETUP(m_event_lock);
    MUTEX_SETUP(m_ref_lock);

#ifndef _WIN32
    pthread_mutexattr_t mux_attr;
    memset(&mux_attr, 0, sizeof(mux_attr));
    pthread_mutexattr_settype(&mux_attr, PTHREAD_MUTEX_RECURSIVE);
    MUTEX_SETUP_ATTR(m_remote_lock, &mux_attr);
#else
    MUTEX_SETUP(m_remote_lock);
#endif
    
    m_refcnt = 0;
    _LOG_DEBUG("construct base connection");
}

CBaseConnection::~CBaseConnection()
{
    MUTEX_CLEANUP(m_event_lock);
    MUTEX_CLEANUP(m_ref_lock);
    MUTEX_CLEANUP(m_remote_lock);
    _LOG_DEBUG("destruct base connection");
}

uint32_t CBaseConnection::get_client_inner_ipaddr()
{
    uint32_t ret = 0;
    MUTEX_LOCK(m_remote_lock);
    if (m_client != NULL)
    {
        ret = m_client->get_inner_ipaddr();
    }
    MUTEX_UNLOCK(m_remote_lock);
    return ret;
}
uint16_t CBaseConnection::get_client_inner_port()
{
    uint16_t ret = 0;
    MUTEX_LOCK(m_remote_lock);
    if (m_client != NULL)
    {
        ret = m_client->get_inner_port();
    }
    MUTEX_UNLOCK(m_remote_lock);
    return ret;
}

uint32_t CBaseConnection::get_client_ipaddr()
{
    uint32_t ret = 0;
    MUTEX_LOCK(m_remote_lock);
    if (m_client != NULL)
    {
        ret = m_client->get_ipaddr();
    }
    MUTEX_UNLOCK(m_remote_lock);
    return ret;
}
uint16_t CBaseConnection::get_client_port()
{
    uint16_t ret = 0;
    MUTEX_LOCK(m_remote_lock);
    if (m_client != NULL)
    {
        ret = m_client->get_port();
    }
    MUTEX_UNLOCK(m_remote_lock);
    return ret;
}
int CBaseConnection::get_client_fd()
{
    int ret = -1;
    MUTEX_LOCK(m_remote_lock);
    if (m_client != NULL)
    {
        ret = m_client->get_fd();
    }
    MUTEX_UNLOCK(m_remote_lock);
    return ret;
}

uint32_t CBaseConnection::get_remote_ipaddr()
{
    uint32_t ret = 0;
    MUTEX_LOCK(m_remote_lock);
    if (m_remote != NULL)
    {
        ret = m_remote->get_ipaddr();
    }
    MUTEX_UNLOCK(m_remote_lock);
    return ret;
}
uint16_t CBaseConnection::get_remote_port()
{
    uint16_t ret = 0;
    MUTEX_LOCK(m_remote_lock);
    if (m_remote != NULL)
    {
        ret = m_remote->get_port();
    }
    MUTEX_UNLOCK(m_remote_lock);
    return ret;
}
int CBaseConnection::get_remote_fd()
{
    int ret = -1;
    MUTEX_LOCK(m_remote_lock);
    if (m_remote != NULL)
    {
        ret = m_remote->get_fd();
    }
    MUTEX_UNLOCK(m_remote_lock);
    return ret;
}

int CBaseConnection::fwd_client_data_msg(char *buf, int buf_len)
{
    int ret = -1;
    MUTEX_LOCK(m_remote_lock);
    if (m_remote != NULL)
    {
        if (buf_len > FRAME_LEN)
        {
            ret = m_remote->send_data_msg(buf, FRAME_LEN);
            ret |= m_remote->send_data_msg(buf+FRAME_LEN, buf_len-FRAME_LEN);
        }
        else
        {
            ret = m_remote->send_data_msg(buf, buf_len);
        }

        _LOG_DEBUG("client(%s/%u/%s/%u/fd%d) forward data to remote, len %d", 
            m_client->get_ipstr(), m_client->get_port(), 
            m_client->m_inner_ipstr, m_client->m_inner_port,
            m_client->get_fd(), buf_len);
        m_send_client_data_cnt++;
        m_send_client_bytes+=buf_len;

        g_total_data_req_cnt++;
        g_total_data_req_byte += buf_len;
    }
    else
    {
        _LOG_WARN("remote NULL when forward data to remote");
    }
    MUTEX_UNLOCK(m_remote_lock);

    
    return ret;
}

int CBaseConnection::fwd_remote_data_msg(char *buf, int buf_len)
{
    int ret = -1;

    MUTEX_LOCK(m_remote_lock);
    if (m_client != NULL)
    {
        if (buf_len > FRAME_LEN)
        {
            ret = m_client->send_data_msg(buf, FRAME_LEN);
            ret |= m_client->send_data_msg(buf+FRAME_LEN, buf_len-FRAME_LEN);
        }
        else
        {
            ret = m_client->send_data_msg(buf, buf_len);
        }
        
        _LOG_DEBUG("client(%s/%u/%s/%u/fd%d) forward data to client, len %d", 
            m_client->get_ipstr(), m_client->get_port(), 
            m_client->m_inner_ipstr, m_client->m_inner_port,
            m_client->get_fd(), buf_len);
        m_send_remote_data_cnt++;
        m_send_remote_bytes+=buf_len;

        g_total_data_resp_cnt++;
        g_total_data_resp_byte += buf_len;
    }
    else
    {
        _LOG_WARN("client NULL when forward data to client");
    }
    MUTEX_UNLOCK(m_remote_lock);

    return ret;
}

void CBaseConnection::notify_remote_close()
{
    MUTEX_LOCK(m_remote_lock);
    if (NULL != m_client)
    {
        m_client->send_remote_close_msg();
    }
    MUTEX_UNLOCK(m_remote_lock);
}

void CBaseConnection::notify_client_close()
{
    MUTEX_LOCK(m_remote_lock);
    if (NULL != m_remote)
    {
        m_remote->send_client_close_msg();
    }
    MUTEX_UNLOCK(m_remote_lock);
}

void CBaseConnection::free_client()
{
    MUTEX_LOCK(m_remote_lock);
    if (NULL != m_client)
    {
        m_client->free();
    }
    MUTEX_UNLOCK(m_remote_lock);
}
void CBaseConnection::free_remote()
{
    MUTEX_LOCK(m_remote_lock);
    if (NULL != m_remote)
    {
        m_remote->free();
    }
    MUTEX_UNLOCK(m_remote_lock);
}

void CBaseConnection::inc_ref()
{
    MUTEX_LOCK(m_ref_lock);
    this->m_refcnt++;
    MUTEX_UNLOCK(m_ref_lock);
}


void CBaseConnection::dec_ref()
{
    BOOL is_del = false;

    MUTEX_LOCK(m_ref_lock);
    if (0 == this->m_refcnt)
    {
        MUTEX_UNLOCK(m_ref_lock);
        _LOG_ERROR("ref cnt already zero");
        return;
    }
    else
    {
        this->m_refcnt--;
    }
    if (this->m_refcnt == 0)
    {
        is_del = true;
    }
    MUTEX_UNLOCK(m_ref_lock);

    /*try to del*/
    if (is_del)
    {
        g_ConnMgr->del_conn(this);
        delete this;
    }
}


void CBaseConnection::detach_client()
{
    if (NULL == m_client)
    {
        _LOG_ERROR("has no client");
        return;
    }
    MUTEX_LOCK(m_remote_lock);
    m_client = NULL;
    MUTEX_UNLOCK(m_remote_lock);

    this->dec_ref();
    _LOG_DEBUG("dettach client");
}

void CBaseConnection::detach_remote()
{
    if (NULL == m_remote)
    {
        _LOG_ERROR("has no client");
        return;
    }

    MUTEX_LOCK(m_remote_lock);
    m_remote = NULL;
    MUTEX_UNLOCK(m_remote_lock);

    this->dec_ref();
    _LOG_DEBUG("dettach remote");
}

int CBaseConnection::attach_client(CBaseClient *client)
{
    if (NULL != m_client)
    {
        _LOG_ERROR("already has client");
        return -1;
    }

    this->inc_ref();
    m_client = client;
    _LOG_DEBUG("attach client");
    return 0;
}

int CBaseConnection::attach_remote(CBaseRemote *remote)
{
    if (NULL != m_remote)
    {
        _LOG_ERROR("already has remote");
        return -1;
    }

    this->inc_ref();
    m_remote = remote;
    _LOG_DEBUG("attach remote");
    return 0;
}

void CBaseConnection::destroy()
{
    free_client();
    free_remote();
    _LOG_INFO("destroy connection");
    return;
}


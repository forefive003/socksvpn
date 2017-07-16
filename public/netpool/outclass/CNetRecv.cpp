

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "netpool.h"
#include "socketwrap.h"
#include "CNetRecv.h"

CNetRecv::CNetRecv(char *ipstr, uint16_t port, int fd) throw(std::runtime_error)
{
    if (-1 == init_peer_ipinfo(ipstr, port))
    {
        throw(std::runtime_error("Invalid server ip"));
    }

    m_fd = fd;
    init_common_data();
    _LOG_DEBUG("construct NetRecv(%s/%u), fd %d", m_ipstr, m_port, m_fd);
}

CNetRecv::CNetRecv(uint32_t ipaddr, uint16_t port, int fd) throw(std::runtime_error)
{
    if (-1 == init_peer_ipinfo(ipaddr, port))
    {
        throw(std::runtime_error("Invalid server ip"));
    }

    m_fd = fd;
    init_common_data();
    _LOG_DEBUG("construct1 NetRecv(%s/%u), fd %d", m_ipstr, m_port, m_fd);
}

CNetRecv::CNetRecv(uint32_t ipaddr, uint16_t port) throw(std::runtime_error)
{
    if (-1 == init_peer_ipinfo(ipaddr, port))
    {
        throw(std::runtime_error("Invalid server ip"));
    }

    m_fd = -1;
    init_common_data();
    _LOG_DEBUG("construct2 NetRecv(%s/%u)", m_ipstr, m_port, m_fd);
}


CNetRecv::CNetRecv(char *ipstr, uint16_t port) throw(std::runtime_error)
{
    if (-1 == init_peer_ipinfo(ipstr, port))
    {
        throw(std::runtime_error("Invalid server ip"));
    }

    m_fd = -1;
    init_common_data();
    _LOG_DEBUG("construct3 NetRecv(%s/%u)", m_ipstr, m_port, m_fd);
}


CNetRecv::~CNetRecv()
{
    destroy_common_data();
    _LOG_DEBUG("destruct NetRecv");
}

void CNetRecv::init_common_data()
{
    m_thrd_index = 0;

    m_is_connected = false;
    
    m_is_register_write = false;
    m_is_register_read = false;
    m_is_register_connect = false;

    m_new_mem_func = NULL;
    m_free_mem_func = NULL;
    m_is_async_write = false;
    pthread_spin_init(&m_queue_lock, 0);
    m_is_write_failed = false;

    MUTEX_SETUP(m_register_lock);
    m_is_freed = false;
}

void CNetRecv::destroy_common_data()
{
    pthread_spin_destroy(&m_queue_lock);
    MUTEX_CLEANUP(m_register_lock);
}

int CNetRecv::init_peer_ipinfo(uint32_t ipaddr, uint16_t port)
{
    m_ipaddr = ipaddr;
    m_port = port;

    struct sockaddr_storage ipaddr_s;
    memset((void*) &ipaddr_s, 0, sizeof(struct sockaddr_storage));
    ipaddr_s.ss_family = AF_INET;
    ((struct sockaddr_in *) &ipaddr_s)->sin_addr.s_addr = htonl(ipaddr);
    if (NULL == util_ip_to_str(&ipaddr_s, m_ipstr)) 
    {
        _LOG_ERROR("ip to str failed.");
        return -1;
    }
    m_ipstr[HOST_IP_LEN] = 0;
    return 0;
}

int CNetRecv::init_peer_ipinfo(char *ipstr, uint16_t port)
{
    strncpy(m_ipstr, ipstr, HOST_IP_LEN);
    m_ipstr[HOST_IP_LEN] = 0;
    m_port = port;

    struct sockaddr_storage ipaddr;
    if (0 != util_str_to_ip(ipstr, &ipaddr))
    {
        _LOG_ERROR("Invalid server %s", ipstr);
        return -1;
    }
    m_ipaddr = htonl(((struct sockaddr_in *)&ipaddr)->sin_addr.s_addr);
    return 0;
}

int CNetRecv::init_local_ipinfo()
{
    struct sockaddr_in name;
    socklen_t len = sizeof (struct sockaddr);  
    if (getsockname (m_fd, (struct sockaddr *)&name, &len) == -1) {
        return -1;
    }     
    m_local_ipaddr = ntohl(name.sin_addr.s_addr);
    m_local_port = ntohs(name.sin_port);

    struct sockaddr_storage ipaddr_s;
    memset((void*) &ipaddr_s, 0, sizeof(struct sockaddr_storage));
    ipaddr_s.ss_family = AF_INET;
    ((struct sockaddr_in *) &ipaddr_s)->sin_addr.s_addr = htonl(m_local_ipaddr);
    if (NULL == util_ip_to_str(&ipaddr_s, m_local_ipstr)) 
    {
        _LOG_ERROR("ip to str failed.");
        return -1;
    }
    m_local_ipstr[HOST_IP_LEN] = 0;

    _LOG_DEBUG("(%s/%u) fd %d get local ip : %s/%u", m_ipstr, m_port, m_fd, m_local_ipstr, m_local_port);
    return 0;
}

int CNetRecv::connect_handle(BOOL result)
{
    _LOG_ERROR("(%s/%u) fd %d default connect handle, shouldn't be here", m_ipstr, m_port, m_fd);
    return -1;
}

void CNetRecv::free_handle()
{
    /*default, delete self*/
    delete this;
}

void CNetRecv::_send_callback(int  fd, void* param1)
{
    CNetRecv *recvObj = (CNetRecv*)param1;

    if (recvObj->m_fd != fd)
    {
        _LOG_ERROR("(%s/%u) fd %d invalid fd when recv", recvObj->m_ipstr, recvObj->m_port, fd);
        recvObj->free();
        return;
    }

    if (-1 == recvObj->send_handle())
    {
        _LOG_ERROR("(%s/%u) fd %d send handle failed", recvObj->m_ipstr, recvObj->m_port, fd);
        recvObj->free();
        return;
    }

    return;
}

void CNetRecv::_recv_callback(int  fd, void* param1, char *recvBuf, int recvLen)
{
    CNetRecv *recvObj = (CNetRecv*)param1;

    if (recvObj->m_fd != fd)
    {
        _LOG_ERROR("(%s/%u) fd %d invalid fd when recv", recvObj->m_ipstr, recvObj->m_port, fd);
        recvObj->free();
        return;
    }

    if (recvLen == 0)
    {
        _LOG_INFO("(%s/%u, inner %s/%u) fd %d close.", recvObj->m_ipstr, recvObj->m_port, 
            recvObj->m_local_ipstr, recvObj->m_local_port, fd);
        recvObj->free();
        return;
    }

    if (-1 == recvObj->recv_handle(recvBuf, recvLen))
    {
        _LOG_ERROR("(%s/%u) fd %d recv handle failed", recvObj->m_ipstr, recvObj->m_port, fd);
        recvObj->free();
        return;
    }

    return;
}

void CNetRecv::_free_callback(int  fd, void* param1)
{
    CNetRecv *recvObj = (CNetRecv*)param1;
    if (recvObj->m_fd != fd)
    {
        _LOG_ERROR("(%s/%u) fd %d invalid when free", recvObj->m_ipstr, recvObj->m_port, fd);
        recvObj->free();
        return;
    }
    
    _LOG_INFO("(%s/%u inner %s/%u) fd %d call free callback", recvObj->m_ipstr, recvObj->m_port, 
        recvObj->m_local_ipstr, recvObj->m_local_port,
        recvObj->m_fd);

    recvObj->free_async_write_resource();
    close(recvObj->m_fd);
    recvObj->m_fd = -1;

    /*调用上层的释放函数, 用户在自己的freehandle中delete self*/
    recvObj->free_handle();
}


void CNetRecv::_connect_callback(int  fd, void* param1)
{
    CNetRecv *recvObj = (CNetRecv*)param1;

    if (recvObj->m_fd != fd)
    {
        _LOG_ERROR("(%s/%u) fd %d invalid fd when connect, wanted %d", 
            recvObj->m_ipstr, recvObj->m_port, fd, recvObj->m_fd);
        recvObj->free();
        return;
    }

    int ret = 0;
    int err = 0;
    socklen_t err_len = sizeof (err);
    
    ret = getsockopt(recvObj->m_fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
    if (err == 0 && ret == 0)
    {
        _LOG_INFO("(%s/%u) fd %d connected.", recvObj->m_ipstr, recvObj->m_port, recvObj->m_fd);
        recvObj->connect_handle(true);
        recvObj->m_is_connected = true;
    }
    else
    {
        char err_buf[64] = {0};
        _LOG_WARN("(%s/%u) fd %d connect failed, err:%s", 
            recvObj->m_ipstr, recvObj->m_port, recvObj->m_fd, str_error_s(err_buf, 32, err));
        recvObj->connect_handle(false);
    }
    
    /*unregister connect event if no write event register*/
    if (false == recvObj->m_is_register_write
        && true == recvObj->m_is_register_connect)
    {
        recvObj->unregister_connect();
    }
    return;
}

int CNetRecv::register_read()
{
    if (m_fd == -1)
    {
        _LOG_ERROR("no fd when init NetRecv, %s/%u", m_ipstr, m_port);
        return -1;
    }
    
    /*initialize local ipaddr and port*/
    init_local_ipinfo();

    sock_set_unblock(m_fd);
    if(false == np_add_read_job(CNetRecv::_recv_callback, m_fd, (void*)this, m_thrd_index))
    {
        return -1;
    }

    m_is_register_read = true;
    return 0;
}

void CNetRecv::unregister_read()
{
    np_del_read_job(m_fd, CNetRecv::_free_callback);
    m_is_register_read = false;
}

int CNetRecv::register_connect()
{
    if ((m_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        _LOG_ERROR("create socket failed");
        return -1;
    }

    /*set to nonblock*/
    sock_set_unblock(m_fd);

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(m_port);
    sa.sin_addr.s_addr = htonl(m_ipaddr); 
    if (connect(m_fd, (struct sockaddr*)&sa, sizeof(sa)) == 0) 
    {
        m_is_connected = true;
        this->connect_handle(true);
        return 0;
    }
    
    if (errno != EINPROGRESS)
    {
        _LOG_ERROR("connect failed, %s:%u %s", m_ipstr, m_port, strerror(errno));
        close(m_fd);
        m_fd = -1;
        return -1;
    }

    m_is_register_connect = true;
    if(false == np_add_write_job(CNetRecv::_connect_callback, m_fd, (void*)this, m_thrd_index))
    {
        return -1;
    }

    return 0;
}

void CNetRecv::unregister_connect()
{
    np_del_write_job(m_fd, CNetRecv::_free_callback);
    m_is_register_connect = false;
}

int CNetRecv::register_write()
{
    if(false == np_add_write_job(CNetRecv::_send_callback, m_fd, (void*)this, m_thrd_index))
    {
        return -1;
    }

    m_is_register_write = true;
    return 0;
}

void CNetRecv::unregister_write()
{
    np_del_write_job(m_fd, CNetRecv::_free_callback);
    m_is_register_write = false;
}


void CNetRecv::init_async_write_resource(void* (*new_mem_func)(),
            void (*free_mem_func)(void*))
{
    m_is_write_failed = false;
    m_is_async_write = true;
    SQUEUE_INIT_HEAD(&m_data_queue);
    m_cur_send_node = NULL;

    m_new_mem_func = new_mem_func;
    m_free_mem_func = free_mem_func;
}

void CNetRecv::free_async_write_resource()
{
    pthread_spin_lock(&m_queue_lock);
    if (m_is_async_write)
    {
        /*有可能其他线程会写队列, 而本对象不会在出发write事件,因此不会取队列*/
        m_is_async_write = false;

        int spare_len = 0;
        if (m_is_write_failed == false)
        {
            /*先保证正常情况下所有buf发送完*/
            if (m_cur_send_node != NULL)
            {
                spare_len = m_cur_send_node->produce_pos - m_cur_send_node->consume_pos;
                if ( spare_len != sock_write_timeout(m_fd, 
                                        &m_cur_send_node->data[m_cur_send_node->consume_pos], 
                                        spare_len, 
                                        DEF_WR_TIMEOUT))
                {
                    char err_buf[64] = {0};
                    _LOG_ERROR("(%s/%u inner %s/%u) fd %d send failed, %s.", 
                        m_ipstr, m_port, m_local_ipstr, m_local_port, m_fd,
                        str_error_s(err_buf, 32, errno));

                    m_free_mem_func(m_cur_send_node);
                    m_cur_send_node = NULL;
                    goto free_nodes;
                }
                else
                {
                    m_free_mem_func(m_cur_send_node);
                    m_cur_send_node = NULL;
                }
            }

            buf_node_t* buf_node = NULL;
            while((buf_node = (buf_node_t*)squeue_deq(&m_data_queue)) != NULL)
            {
                spare_len = buf_node->produce_pos - buf_node->consume_pos;
                if ( spare_len != sock_write_timeout(m_fd, 
                                        &buf_node->data[buf_node->consume_pos], 
                                        spare_len, 
                                        DEF_WR_TIMEOUT))
                {
                    char err_buf[64] = {0};
                    _LOG_ERROR("(%s/%u inner %s/%u) fd %d send failed, %s.", 
                        m_ipstr, m_port, m_local_ipstr, m_local_port, m_fd,
                        str_error_s(err_buf, 32, errno));

                    m_free_mem_func((void*)buf_node);
                    goto free_nodes;
                }
                else
                {
                    m_free_mem_func((void*)buf_node);
                }
            }
        }

free_nodes:            
        /*free all memory in queue*/
        int count = 0;
        struct list_head* queue_node = NULL;
        while((queue_node = squeue_deq(&m_data_queue)) != NULL)
        {
            m_free_mem_func(queue_node);
            count++;
        }

        if (m_cur_send_node != NULL)
        {
            m_free_mem_func(m_cur_send_node);
            count++;
        }    

        if (count > 0)
        {
            _LOG_ERROR("(%s/%u, inner %s/%u) fd %d has %d uncompleted write node for write failed.", 
                m_ipstr, m_port, 
                m_local_ipstr,  m_local_port, m_fd, count);
        }
    }

    pthread_spin_unlock(&m_queue_lock);
}

int CNetRecv::send_buf_node(buf_node_t *buf_node)
{  
    int spare_len = buf_node->produce_pos - buf_node->consume_pos;
    int send_len = 0;
    int total_send_len = 0;

    while(spare_len > 0)
    {
        send_len = send(m_fd, &buf_node->data[buf_node->consume_pos], spare_len, 0);
        if (send_len < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                break;
            }
            else if (errno == EINTR)
            { 
                _LOG_WARN("EINTR occured, continue write");
                continue;
            }
            else
            {
                char err_buf[64] = {0};
                _LOG_ERROR("(%s/%u inner %s/%u) fd %d send failed, %s.", 
                        m_ipstr, m_port, m_local_ipstr, m_local_port, m_fd,
                        str_error_s(err_buf, 32, errno));
                /*返回-1后直接free, 类似recv处理失败*/
                return -1;
            }
        }
        else
        {
            assert(spare_len >= send_len);
            spare_len -= send_len;
            buf_node->consume_pos += send_len;
            total_send_len += send_len;
        }
    }

    return total_send_len;
}

int CNetRecv::send_handle()
{
    int count = 0;
    uint32_t bytes = 0;
    int ret = 0;

    if (m_cur_send_node == NULL)
    {
        /*没有节点, 取一个*/
        pthread_spin_lock(&m_queue_lock);
        m_cur_send_node = (buf_node_t*)squeue_deq(&m_data_queue);
        pthread_spin_unlock(&m_queue_lock);
    }

    while(m_cur_send_node != NULL)
    {
        ret = send_buf_node(m_cur_send_node);
        if (-1 == ret)
        {
            /*设置是否发送失败, 如失败, free callback中直接释放内存, 否则先保证所有内存发送完*/
            m_is_write_failed = true;
            return -1;
        }

        bytes += ret;
        count++;

        if (m_cur_send_node->consume_pos == m_cur_send_node->produce_pos)
        {
            /*此buffer节点已经写完, 释放*/
            m_free_mem_func(m_cur_send_node);
            m_cur_send_node = NULL;

            /*再取一个节点*/
            pthread_spin_lock(&m_queue_lock);
            m_cur_send_node = (buf_node_t*)squeue_deq(&m_data_queue);
            pthread_spin_unlock(&m_queue_lock);
        }
        else
        {
            /*没发送完, 下个可写事件时再发*/
            _LOG_DEBUG("(%s/%u, inner %s/%u) fd %d not send finished, wait next.", 
                    m_ipstr, m_port, 
                    m_local_ipstr,  m_local_port, m_fd);
            this->register_write();
            break;
        }
    }

    if (count > 3)
    {
        _LOG_INFO("(%s/%u, inner %s/%u) fd %d async write %d node, bytes %u.", 
            m_ipstr, m_port, 
            m_local_ipstr,  m_local_port, m_fd, count, bytes);
    }

    _LOG_DEBUG("(%s/%u, inner %s/%u) fd %d async write %d node, bytes %u.", 
        m_ipstr, m_port, 
        m_local_ipstr,  m_local_port, m_fd, count, bytes);

    return 0;
}

int CNetRecv::send_data(char *buf, int buf_len)
{
    pthread_spin_lock(&m_queue_lock);
    if (m_is_async_write == false)
    {
        pthread_spin_unlock(&m_queue_lock);

        if ( buf_len != sock_write_timeout(m_fd, buf, buf_len, DEF_WR_TIMEOUT))
        {
            char err_buf[64] = {0};
            _LOG_ERROR("(%s/%u inner %s/%u) fd %d send failed, %s.", 
                m_ipstr, m_port, m_local_ipstr, m_local_port, m_fd,
                str_error_s(err_buf, 32, errno));
            return -1;
        }
        return 0;
    }

    buf_node_t *buf_node = NULL;
    buf_node = (buf_node_t*)squeue_get_tail(&m_data_queue);
    if (NULL == buf_node)
    {
        /*分配一个节点*/
        buf_node = (buf_node_t*)m_new_mem_func();
        if (NULL == buf_node)
        {
            pthread_spin_unlock(&m_queue_lock);
            _LOG_ERROR("send to (%s/%u) failed, fd %d, no buffer.", m_ipstr, m_port, m_fd);
            return -1;
        }
        SQUEUE_INIT_NODE(&buf_node->node);
        buf_node->produce_pos = 0;
        buf_node->consume_pos = 0;

        /*插入到queue中*/
        squeue_inq((struct list_head*)(buf_node), &m_data_queue);
    }

    /*数据写入*/
    int32_t fill_len = 0;
    while(fill_len < buf_len)
    {
        uint32_t buf_spare_len = BUF_NODE_SIZE - buf_node->produce_pos;
        uint32_t send_spare_len = buf_len - fill_len;
        if (send_spare_len > buf_spare_len)
        {
            memcpy(&buf_node->data[buf_node->produce_pos], &buf[fill_len], buf_spare_len);            
            buf_node->produce_pos += buf_spare_len;
            fill_len += buf_spare_len;

            /*再分配一个节点*/
            buf_node = (buf_node_t*)m_new_mem_func();
            if (NULL == buf_node)
            {
                pthread_spin_unlock(&m_queue_lock);
                _LOG_ERROR("send to (%s/%u) failed, fd %d, no buffer.", m_ipstr, m_port, m_fd);
                return -1;
            }

            SQUEUE_INIT_NODE(&buf_node->node);
            buf_node->produce_pos = 0;
            buf_node->consume_pos = 0;

            /*插入到queue中*/
            squeue_inq((struct list_head*)(buf_node), &m_data_queue);
        }
        else
        {
            memcpy(&buf_node->data[buf_node->produce_pos], &buf[fill_len], send_spare_len);
            buf_node->produce_pos += send_spare_len;
            break;
        }
    }

    pthread_spin_unlock(&m_queue_lock);

    /*maybe other thread call free at the same time, cause m_fd register on write, and this obj
        be deleted*/
    int ret = 0;
    MUTEX_LOCK(m_register_lock);
    if(m_is_freed)
    {
        MUTEX_UNLOCK(m_register_lock);
        return -1;
    }
    ret = this->register_write();
    MUTEX_UNLOCK(m_register_lock);

    return ret;
}

void CNetRecv::set_thrd_index(unsigned int thrd_index)
{
    /*都自动均衡使用线程资源*/
    //m_thrd_index = thrd_index;
}

BOOL CNetRecv::is_connected()
{
    return m_is_connected;
}

int CNetRecv::init()
{
    if (-1 != m_fd)
    {
        return register_read();
    }

    /*need to connect peer*/
    return register_connect();
}

void CNetRecv::free()
{
    MUTEX_LOCK(m_register_lock);
    m_is_freed = true;

    if (m_is_register_read)
    {
        unregister_read();
    }

    if (m_is_register_write)
    {
        unregister_write();
    }
    else if (m_is_register_connect)
    {
        unregister_connect();
    }

    MUTEX_UNLOCK(m_register_lock);
}

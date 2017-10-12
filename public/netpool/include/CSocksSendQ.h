
#ifndef _SOCKS_SENDQ_H
#define _SOCKS_SENDQ_H

#include <stdexcept>
#include "list.h"
#include "squeue.h"

#define BUF_NODE_SIZE 1500

typedef struct 
{
    struct list_head node;

    /* data */
    uint16_t consume_pos;
    uint16_t produce_pos;
    char data[BUF_NODE_SIZE];
}buf_node_t;


#ifdef _WIN32

#if  defined(DLL_EXPORT_NP)
class _declspec(dllexport) CSocksSendQ
#elif defined(DLL_IMPORT_NP)
class _declspec(dllimport) CSocksSendQ
#else
class CSocksSendQ
#endif

#else
class CSocksSendQ
#endif
{
public:
    CSocksSendQ()
    {
        m_cur_send_node = NULL;
        SQUEUE_INIT_HEAD(&m_data_queue);
    #ifdef _WIN32
        m_queue_lock = 0;
    #else
        pthread_spin_init(&m_queue_lock, 0);
    #endif
    }

    virtual ~CSocksSendQ()
    {
    #ifdef _WIN32
        m_queue_lock = 0;
    #else
        pthread_spin_destroy(&m_queue_lock);
    #endif

        if (this->node_cnt() > 0)
        {
            _LOG_WARN("%d uncompleted write node when free sendQ.", this->node_cnt());
            this->clean_q();
        }
    }

public:
	int produce_q(char *buf, int buf_len);
    int consume_q(int fd);
    void clean_q();

    void queue_cat(CSocksSendQ &qobj);

	unsigned int node_cnt();
	
    void lock()
    {
    #ifdef _WIN32
        while (InterlockedExchange(&m_queue_lock, 1) == 1){
            sleep_s(0);
        }
    #else
        pthread_spin_lock(&m_queue_lock);
    #endif
    }
    
    void unlock()
    {        
    #ifdef _WIN32
        InterlockedExchange(&m_queue_lock, 0);
    #else
        pthread_spin_unlock(&m_queue_lock);
    #endif
    }

private:
    int send_buf_node(int fd, buf_node_t *buf_node);
    
private:

#ifdef _WIN32
    LONG m_queue_lock;
#else
    pthread_spinlock_t m_queue_lock;
#endif
    sq_head_t m_data_queue;
    buf_node_t* m_cur_send_node;
};

#endif

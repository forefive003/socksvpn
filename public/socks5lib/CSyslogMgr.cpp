#include "commtype.h"
#include "logproc.h"
#include "CSyslogMgr.h"


CSyslogMgr *g_syslogMgr = NULL;

CSyslogMgr::CSyslogMgr()
{
    MUTEX_SETUP(m_obj_lock);
}

CSyslogMgr::~CSyslogMgr()
{
    MUTEX_CLEANUP(m_obj_lock);
}

int CSyslogMgr::add_syslog(int level, const char *format, ...)
{
    LOG_MSG_T *logNode = NULL;

    if (m_logs.size() >= LOG_MSG_MAX_CNT)
    {
        MUTEX_LOCK(m_obj_lock);
        logNode = m_logs.front();
        m_logs.pop_front();
        MUTEX_UNLOCK(m_obj_lock);

        delete logNode;
    }

    logNode = new LOG_MSG_T;
    memset(logNode, 0, sizeof(logNode));

    logNode->nodetime = util_get_cur_time();
    logNode->level = level;

    va_list argp;
    va_start(argp, format);
    VSNPRINTF(logNode->data, LOG_MSG_MAX_LEN, format, argp);
    va_end(argp);

    MUTEX_LOCK(m_obj_lock);
    m_logs.push_back(logNode);
    MUTEX_UNLOCK(m_obj_lock);
    return 0;
}

int CSyslogMgr::add_syslog(int level, char *logdata)
{
    LOG_MSG_T *logNode = NULL;

    if (m_logs.size() >= LOG_MSG_MAX_CNT)
    {
        MUTEX_LOCK(m_obj_lock);
        logNode = m_logs.front();
        m_logs.pop_front();
        MUTEX_UNLOCK(m_obj_lock);

        delete logNode;
    }

    logNode = new LOG_MSG_T;
    memset(logNode, 0, sizeof(logNode));

    logNode->nodetime = util_get_cur_time();
    logNode->level = level;
    
    strncpy(logNode->data, logdata, LOG_MSG_MAX_LEN);

    MUTEX_LOCK(m_obj_lock);
    m_logs.push_back(logNode);
    MUTEX_UNLOCK(m_obj_lock);
    return 0;
}

void CSyslogMgr::aged_syslog()
{
    uint64_t agedtime = util_get_cur_time();
    agedtime -= LOG_MSG_AGE_TIME;

	LogMsgListRItr itr;
    LOG_MSG_T *logNode = NULL;

    //MUTEX_LOCK(m_obj_lock);
    for (itr = m_logs.rbegin();
            itr != m_logs.rend(); )
    {
        logNode = *itr;
        itr++;

        if (logNode->nodetime >= agedtime)
        {
            break;
        }

        m_logs.remove(logNode);
        delete logNode;
    }
    //MUTEX_UNLOCK(m_obj_lock);

    return;
}

int CSyslogMgr::init()
{
    return 0;
}

void CSyslogMgr::free()
{
    return;
}

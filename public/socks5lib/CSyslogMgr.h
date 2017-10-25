
#ifndef _SYSLOG_MGR_H
#define _SYSLOG_MGR_H

#include <list>

#define LOG_MSG_MAX_CNT   100
#define LOG_MSG_MAX_LEN   1024

typedef struct
{
    uint64_t nodetime;
    char data[LOG_MSG_MAX_LEN];
}LOG_MSG_T;

typedef std::list<LOG_MSG_T*> LogMsgList;
typedef LogMsgList::iterator LogMsgListItr;
typedef LogMsgList::reverse_iterator LogMsgListRItr; /*反向遍历迭代器*/

class CSyslogMgr
{
public:
    CSyslogMgr();
    virtual ~CSyslogMgr();
    
public:
    int init();
    void free();

    int add_syslog(const char *format, ...);
    int add_syslog(char *logdata);

    void aged_syslog();

public:
    MUTEX_TYPE m_obj_lock;
    LogMsgList m_logs;
};

extern CSyslogMgr* g_syslogMgr;

#endif

#ifndef __THREAD_I_H__
#define __THREAD_I_H__

#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include   <windows.h>
#include   <process.h>
#else
#include <sys/prctl.h>
#include <pthread.h>
#endif


#define NO_SPEC_CORE 0xffff

class Runnable
{
public:
    virtual ~Runnable() {};
    virtual void Run() = 0;
};

class CThread : public Runnable
{
private:
    explicit CThread(const CThread & rhs);

public:
    CThread();
    CThread(Runnable * pRunnable);
    ~CThread(void);

    BOOL Start(unsigned int core_id);
    virtual void Run();

    void SetExitFlag();
    void volatile Join(int timeout);
    BOOL Terminate(unsigned long ExitCode);

    unsigned long int GetThreadID();

private:
#ifdef _WIN32
    static unsigned int WINAPI StaticThreadFunc(void * arg);
#else
    static void* StaticThreadFunc(void * arg);
#endif

private:
#ifdef _WIN32
    HANDLE m_handle;
#else
    pthread_t m_handle;
#endif

    Runnable * const m_pRunnable;
    unsigned int m_ThreadID;
    volatile BOOL m_bRun;
};

#endif

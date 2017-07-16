
#ifndef COMMONTYPE_H_
#define COMMONTYPE_H_

#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef _WIN32
#include <windows.h>
#include "stdafx.h"
#include <direct.h>
#include <io.h>
#ifdef ETP_DLL_EXPORT
#define DLL_API __declspec(dllexport)
#elif defined(ETP_USE_DLL)
#define DLL_API __declspec(dllimport)
#elif defined(ETP_USE_LIB)
#define DLL_API 
#else
#define DLL_API 
#endif

#else
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h> //has uint64_t defined
#define DLL_API extern
#endif


#ifdef _WIN32
#define _COM_TYPES_
#endif

#ifndef _COM_TYPES_
typedef int 			BOOL;

//typedef char            int8_t;
typedef short           int16_t;
typedef int             int32_t;

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;

//typedef long                int64_t;
//typedef unsigned long       uint64_t;

#ifndef OK
#define OK 0
#endif

#ifndef ERROR
#define ERROR -1
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif

#define inline __inline



#ifdef _WIN32

//#define EINVAL  WSAEINVAL
//#define EAGAIN  WSAEWOULDBLOCK
//#define EINTR   WSAEINTR
//#define EWOULDBLOCK   WSAEWOULDBLOCK
//#define EACCES   WSAEACCES


#define MUTEX_TYPE            CRITICAL_SECTION
#define MUTEX_SETUP(x)        InitializeCriticalSection(&(x))
#define MUTEX_SETUP_ATTR(x, attr)        InitializeCriticalSection(&(x))
#define MUTEX_CLEANUP(x)      DeleteCriticalSection(&(x))
#define MUTEX_LOCK(x)         EnterCriticalSection(&(x))
#define MUTEX_UNLOCK(x)       LeaveCriticalSection(&(x))
#define THREAD_ID             GetCurrentThreadId()

#define ACCESS _access
#define MKDIR(a) _mkdir((a))
#define STRDUP(a) _strdup(a)


#define VSNPRINTF(buf, len, format, ap)\
		vsnprintf_s(buf, len, _TRUNCATE, format, ap);

#define SNPRINTF(buf, len, format, ...)\
		_snprintf_s(buf, len, _TRUNCATE, format, ##__VA_ARGS__);

typedef int socklen_t;


static inline void sleep_s(int second)
{
	Sleep(second*1000);
}

static inline char* str_error_s(char *err_buf, unsigned int len, int errnum)
{
	SNPRINTF(err_buf, len, "%d", errnum);
	return err_buf;
}

#else

#define MUTEX_TYPE            pthread_mutex_t
#define MUTEX_SETUP(x)        pthread_mutex_init(&(x), NULL)
#define MUTEX_SETUP_ATTR(x, attr)  pthread_mutex_init(&(x), attr)
#define MUTEX_CLEANUP(x)      pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)         pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)       pthread_mutex_unlock(&(x))
#define THREAD_ID             pthread_self()

#define ACCESS access
#define MKDIR(a) mkdir((a),0777)
#define STRDUP(a) strdup(a)

#define VSNPRINTF(buf, len, format,ap)\
		vsnprintf(buf, len, format, ap);

#define SNPRINTF(buf, len, format, ...)\
		snprintf(buf, len, format, ##__VA_ARGS__);


static inline void sleep_s(int second)
{
	sleep(second);
}

static inline char* str_error_s(char *err_buf, unsigned int len, int errnum)
{
	return strerror_r(errnum, err_buf, len);
}
#endif


static inline uint64_t util_get_cur_time()
{
    return time(NULL);
}


#ifdef __cplusplus
}
#endif




#endif /* COMMONTYPE_H_ */

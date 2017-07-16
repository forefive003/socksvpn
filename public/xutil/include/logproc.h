
#ifndef _LOG_PROC_H_
#define _LOG_PROC_H_

#include <string.h>

#ifdef _WIN32

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
#define DLL_API extern
#endif



#ifdef __cplusplus
extern "C"
{
#endif

enum
{
	L_DEBUG = 0,
	L_INFO,
	L_WARN,
	L_ERROR,

	L_MAX,
};

#define _LOG_DEBUG(format, ...) \
		logger_write(L_DEBUG, format, ##__VA_ARGS__);

#define _LOG_ERROR(format, ...) \
		logger_write(L_ERROR, format, ##__VA_ARGS__);

#define _LOG_WARN(format, ...) \
		logger_write(L_WARN, format, ##__VA_ARGS__);

#define _LOG_INFO(format, ...) \
		logger_write(L_INFO, format, ##__VA_ARGS__);


DLL_API int loggger_init(char *log_path, char *mod_name,
					unsigned int maxfilekb, unsigned int maxfilecnt,
					BOOL isAsynWr);
DLL_API void loggger_exit();
DLL_API void logger_write(int level, const char *format, ...);
DLL_API void logger_set_level(int level);


#ifdef __cplusplus
}
#endif


#endif
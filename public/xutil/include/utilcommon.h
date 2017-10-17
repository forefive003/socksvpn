
#ifndef _XUTIL_COMMON_H
#define _XUTIL_COMMON_H

#include "commtype.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_PATHNAME_LEN 256

typedef unsigned long UTIL_TID;

DLL_API int util_get_cpu_core_num();
DLL_API int util_creatdir(char *pDir);
DLL_API UTIL_TID util_get_cur_tid();

#ifdef __cplusplus
}
#endif


#endif

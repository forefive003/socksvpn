#ifndef _UTIL_RAND_H_
#define _UTIL_RAND_H_

#include <stdint.h>
#include "commtype.h"


#ifdef __cplusplus
extern "C"
{
#endif


#define PHI 0x9e3779b9

DLL_API void rand_init(void);
DLL_API uint32_t rand_next(void);
DLL_API void rand_str(char *, uint32_t);
DLL_API void rand_alphastr(char *, uint32_t);

#ifdef __cplusplus
}
#endif

#endif

#ifndef _UTIL_STR_H_
#define _UTIL_STR_H_

#include "commtype.h"


#ifdef __cplusplus
extern "C"
{
#endif
	
DLL_API int util_strlen(char *);
DLL_API int util_strncmp(char *, char *, int);
DLL_API int util_strcmp(char *, char *);
DLL_API int util_strncpy(char *, char *, int);
DLL_API int util_strcpy(char *, char *);
DLL_API void util_memcpy(void *, void *, int);
DLL_API void util_zero(void *, int);
DLL_API int util_memsearch(char *, int, char *, int);
DLL_API int util_stristr(char *, int, char *);

DLL_API char *util_str_trim(char *src, char find);

static inline int util_isupper(char c)
{
    return (c >= 'A' && c <= 'Z');
}

static inline int util_isalpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static inline int util_isspace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

static inline int util_isdigit(char c)
{
    return (c >= '0' && c <= '9');
}

#ifdef __cplusplus
}
#endif

#endif

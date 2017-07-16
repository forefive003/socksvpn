

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "commtype.h"
#include "utilfile.h"

DLL_API int util_get_cpu_core_num()
{
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);  
    return info.dwNumberOfProcessors;  
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

DLL_API int util_creatdir(char *pDir)
{
    int i = 0;
    int iRet;
    int iLen;
    char* pszDir;

    if(NULL == pDir)
    {
        return 0;
    }

    pszDir = STRDUP(pDir);
    iLen = strlen(pszDir);

    if (pszDir[0] == '/')
    {
        /*linuxÏÂµÚÒ»¸öÎª/·ûºÅ*/
        i = 1;
    }
    else
    {
        i = 0;
    }

    // ´´½¨ÖÐ¼äÄ¿Â¼
    for (;i < iLen;i ++)
    {
        if (pszDir[i] == '\\' || pszDir[i] == '/')
        {
            pszDir[i] = '\0';

            //Èç¹û²»´æÔÚ,´´½¨
            iRet = ACCESS(pszDir,0);
            if (iRet != 0)
            {
                iRet = MKDIR(pszDir);
                if (iRet != 0)
                {
                    char errBuf[64] = {0};
                    printf("mkdir %s failed, error %s.\n", pszDir,
                                str_error_s(errBuf, 64, errno));
                    return -1;
                }
            }
            //Ö§³Ölinux,½«ËùÓÐ\»»³É/
            pszDir[i] = '/';
        }
    }

    iRet = MKDIR(pszDir);
    free(pszDir);
    return iRet;
}

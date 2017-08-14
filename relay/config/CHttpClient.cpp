/*
 * CHttpClient.cpp
 *
 *  Created on: 2015��8��26��
 *      Author: cht
 */

#include "CHttpClient.h"
#include "curl.h"
#include "commtype.h"
#include "logproc.h"

#include <openssl/ssl.h>
#include <pthread.h>

static pthread_mutex_t *lockarray = NULL;

static void lock_callback(int mode, int type, const char *file, int line)
{
	(void)file;
	(void)line;
	if (mode & CRYPTO_LOCK)
	{
		pthread_mutex_lock(&(lockarray[type]));
	}
	else
	{
		pthread_mutex_unlock(&(lockarray[type]));
	}
}

static unsigned long thread_id(void)
{
	return (unsigned long)pthread_self();
}

static void init_locks(void)
{
	int i;

	lockarray=(pthread_mutex_t *)malloc(CRYPTO_num_locks() *
											sizeof(pthread_mutex_t));
	for (i=0; i<CRYPTO_num_locks(); i++)
	{
		pthread_mutex_init(&(lockarray[i]),NULL);
	}

	CRYPTO_set_id_callback(thread_id);
	CRYPTO_set_locking_callback(lock_callback);
}

static void kill_locks(void)
{
	int i;

	CRYPTO_set_locking_callback(NULL);
	for (i=0; i<CRYPTO_num_locks(); i++)
	{
		pthread_mutex_destroy(&(lockarray[i]));
	}

	free(lockarray);
}


CHttpClient::CHttpClient() {
	// TODO Auto-generated constructor stub
	m_debug = FALSE;

	/*初始化多线程锁*/
	init_locks();
}

CHttpClient::~CHttpClient() {
	// TODO Auto-generated destructor stub

	/*销毁多线程锁*/
	kill_locks();
}

void CHttpClient::setDebug()
{
	m_debug = TRUE;
}

static void curCodeDesc(int errCode, char *desc)
{
	switch(errCode)
	{
	case CURLE_COULDNT_CONNECT:
		strncpy(desc, "Couldn't Connect", 32);
		desc[31] = 0;
		break;
	case CURLE_OPERATION_TIMEDOUT:
		strncpy(desc, "Connect Timeout", 32);
		desc[31] = 0;
		break;

	default:
		snprintf(desc, 32, "%d", errCode);
		break;
	}

	return;
}

static int OnDebug(CURL *, curl_infotype itype, char * pData, size_t size, void *)
{
    if(itype == CURLINFO_TEXT)
    {
    	_LOG_DEBUG("[TEXT]%s", pData);
    }
    else if(itype == CURLINFO_HEADER_IN)
    {
    	_LOG_INFO("[HEADER_IN]%s", pData);
    }
    else if(itype == CURLINFO_HEADER_OUT)
    {
    	_LOG_INFO("[HEADER_OUT]%s", pData);
    }
    else if(itype == CURLINFO_DATA_IN)
    {
    	_LOG_INFO("[DATA_IN]%s", pData);
    }
    else if(itype == CURLINFO_DATA_OUT)
    {
    	_LOG_INFO("[DATA_OUT]%s", pData);
    }
    else
    {
    	_LOG_INFO("[DATA_OUT]itype %d, %s", itype, pData);
    }

    return 0;
}

static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);
    if( NULL == str || NULL == buffer )
    {
    	_LOG_ERROR("OnWriteData param invalid.");
        return -1;
    }

    char* pData = (char*)buffer;
    str->append(pData, size * nmemb);

//    COLL_LOG_DEBUG("http receive debug: %s.", pData);
    return size * nmemb;
}

static size_t OnWriteFile(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    FILE *pFd = dynamic_cast<FILE*>((FILE*)lpVoid);
    if( NULL == pFd || NULL == buffer )
    {
    	_LOG_ERROR("OnWriteFile param invalid.");
        return -1;
    }

//    fprintf(pFd, "%s", (char*)buffer);

    //COLL_LOG_DEBUG("OnWriteFile %s.", (char*)buffer);
    return fwrite(buffer, size, nmemb, pFd);
}

int CHttpClient::Post(std::string &strUrl,
		std::string &strPost,
		std::string &strResponse)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    if(NULL == curl)
    {
    	_LOG_ERROR("curl post init failed.");
        return -1;
    }

    if (m_debug)
	{
		/*打印调试信息*/
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		/*打印完整的调试信息*/
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
	}

	/*设置连接的url*/
	curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());

	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost.c_str());

	/*需要读取数据传递给远程主机时将调用CURLOPT_READFUNCTION指定的函数
		CURLOPT_READDATA 表明CURLOPT_READFUNCTION函数原型中的stream指针来源。
		设定回调函数的第四个参数的数据类型*/

	/*函数将在libcurl接收到数据后被调用，因此函数多做数据保存的功能，如处理下载文件。
		CURLOPT_WRITEDATA 用于表明CURLOPT_WRITEFUNCTION函数中的stream指针的来源。
		设定回调函数的第四个参数的数据类型*/
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);

	/**
	* 当多个线程都使用超时处理的时候，同时主线程中有sleep或是wait等操作。
	* 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
	*/
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

	/*在发起连接前等待的时间，如果设置为0，则无限等待*/
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
	/*设置允许执行的最长秒数*/
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, POST_TIMEOUT);

#if 0
    /*自定义http头部*/
    struct curl_slist *headers=NULL; /* init to NULL is important */
	headerlist = curl_slist_append(headers, "Hey-server-hey: how are you?");
	headerlist = curl_slist_append(headers, "X-silly-content: yes");
	/* pass our list of custom made headers */
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
#else
	struct curl_slist *http_headers=NULL; /* init to NULL is important */
	http_headers = curl_slist_append(http_headers, "Accept: application/json");
    http_headers = curl_slist_append(http_headers, "Content-Type: application/json");
    http_headers = curl_slist_append(http_headers, "charsets: utf-8");
    code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
#endif

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
    	char errDesc[32] = {0};
    	curCodeDesc(res, errDesc);
    	_LOG_WARN("curl post perform failed, error %s.", errDesc);
    }

    curl_easy_cleanup(curl);
    if (res == CURLE_OK)
	{
		return 0;
	}
	else if (res == CURLE_COULDNT_CONNECT
			|| res == CURLE_OPERATION_TIMEDOUT)
	{
		return HTTP_CONNECT_FAILED;
	}

	return -1;
}

int CHttpClient::Get(const std::string &strUrl, std::string &strResponse)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    if(NULL == curl)
    {
    	_LOG_ERROR("curl get init failed.");
        return -1;
    }

    if (m_debug)
    {
		/*打印调试信息*/
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		/*打印完整的调试信息*/
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }

	/*设置连接的url*/
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());

    /*需要读取数据传递给远程主机时将调用CURLOPT_READFUNCTION指定的函数
		CURLOPT_READDATA 表明CURLOPT_READFUNCTION函数原型中的stream指针来源。
		设定回调函数的第四个参数的数据类型*/

    /*函数将在libcurl接收到数据后被调用，因此函数多做数据保存的功能，如处理下载文件。
    	CURLOPT_WRITEDATA 用于表明CURLOPT_WRITEFUNCTION函数中的stream指针的来源。
    	设定回调函数的第四个参数的数据类型*/
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);

    /**
	* 当多个线程都使用超时处理的时候，同时主线程中有sleep或是wait等操作。
	* 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
	*/
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    /*在发起连接前等待的时间，如果设置为0，则无限等待*/
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
    /*设置允许执行的最长秒数*/
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, GET_TIMEOUT);

#if 0
    /*自定义http头部*/
    struct curl_slist *headers=NULL; /* init to NULL is important */
	headerlist = curl_slist_append(headers, "Hey-server-hey: how are you?");
	headerlist = curl_slist_append(headers, "X-silly-content: yes");
	/* pass our list of custom made headers */
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
#else
	struct curl_slist *http_headers=NULL; /* init to NULL is important */
	http_headers = curl_slist_append(http_headers, "Accept: application/json");
    http_headers = curl_slist_append(http_headers, "Content-Type: application/json");
    http_headers = curl_slist_append(http_headers, "charsets: utf-8");
    code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
#endif

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
	{
    	char errDesc[32] = {0};
		curCodeDesc(res, errDesc);
		_LOG_WARN("curl get perform failed, error %s.", errDesc);
	}

#if 0
    /* clean up head list */
	curl_slist_free_all (headerlist);
#endif

    curl_easy_cleanup(curl);
    if (res == CURLE_OK)
    {
    	return 0;
    }
    else if (res == CURLE_COULDNT_CONNECT
    		|| res == CURLE_OPERATION_TIMEDOUT)
    {
    	return HTTP_CONNECT_FAILED;
    }

    return -1;
}

int CHttpClient::DownLoadFile(const std::string &strUrl, std::string &strFileName)
{
	FILE *pFd = fopen(strFileName.c_str(), "w");
	if (NULL == pFd)
	{
		_LOG_ERROR("open file %s failed.", strFileName.c_str());
		return -1;
	}

	CURLcode res;
	CURL* curl = curl_easy_init();
	if(NULL == curl)
	{
		fclose(pFd);
		_LOG_ERROR("curl get init failed.");
		return -1;
	}

	if (m_debug)
	{
		/*打印调试信息*/
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		/*打印完整的调试信息*/
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
	}

	/*设置连接的url*/
	curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());

	/*需要读取数据传递给远程主机时将调用CURLOPT_READFUNCTION指定的函数
		CURLOPT_READDATA 表明CURLOPT_READFUNCTION函数原型中的stream指针来源。
		设定回调函数的第四个参数的数据类型*/

	/*函数将在libcurl接收到数据后被调用，因此函数多做数据保存的功能，如处理下载文件。
		CURLOPT_WRITEDATA 用于表明CURLOPT_WRITEFUNCTION函数中的stream指针的来源。
		设定回调函数的第四个参数的数据类型*/
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteFile);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)pFd);

	/**
	* 当多个线程都使用超时处理的时候，同时主线程中有sleep或是wait等操作。
	* 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
	*/
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

	/*在发起连接前等待的时间，如果设置为0，则无限等待*/
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
	/*设置允许执行的最长秒数*/
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, DOWNLOAD_TIMEOUT);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		char errDesc[32] = {0};
		curCodeDesc(res, errDesc);
		_LOG_WARN("curl download perform failed, error %s.", errDesc);
	}

	curl_easy_cleanup(curl);

	fclose(pFd);
	if (res == CURLE_OK)
	{
		return 0;
	}
	else if (res == CURLE_COULDNT_CONNECT
			|| res == CURLE_OPERATION_TIMEDOUT)
	{
		return HTTP_CONNECT_FAILED;
	}

	return -1;
}

#if 0
int CHttpClient::Posts(const std::string & strUrl, const std::string & strPost, std::string & strResponse, const char * pCaPath)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    if(NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }

	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    if(NULL == pCaPath)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    }
    else
    {
        //ȱʡ�������PEM�������������ã�����֧��DER
        //curl_easy_setopt(curl,CURLOPT_SSLCERTTYPE,"PEM");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
        curl_easy_setopt(curl, CURLOPT_CAINFO, pCaPath);
    }
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}

int CHttpClient::Gets(const std::string & strUrl, std::string & strResponse, const char * pCaPath)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    if(NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }

	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    if(NULL == pCaPath)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
        curl_easy_setopt(curl, CURLOPT_CAINFO, pCaPath);
    }
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}
#endif

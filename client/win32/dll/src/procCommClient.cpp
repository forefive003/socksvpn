#include "stdafx.h"
#include <stdio.h> 
#include <TCHAR.h>
#include <errno.h>
#include <Ws2tcpip.h>

#include <windows.h>
#include <process.h>
#include "procCommClient.h"
#include "entry.h"
#include "socketwrap.h"

int proc_send_reg_req(reg_resp_body_t *resp_data)
{
	int ret = 0;
    int sockfd = sock_connect("127.0.0.1", DEF_PROC_COMM_SRV_PORT);
    if (-1 == sockfd)
    {
        char tmpStr[256] = {0};
		_snprintf_s(tmpStr, 256, _T("process %u connect to config server port %d, failed, error %d"), 
			_getpid(), DEF_PROC_COMM_SRV_PORT, WSAGetLastError());
        MessageBox(NULL, tmpStr, _T("Error"), MB_OK);
        return -1;
    }

    proc_msg_t msg;
    msg.type = MSG_REG_REQ;
    msg.len = sizeof(reg_req_body_t);

    reg_req_body_t req_body;
    req_body.proc_id = g_cur_proc_pid;

    ret = sock_write_timeout(sockfd, (char*)&msg, sizeof(proc_msg_t), -1);
    if (ret != sizeof(proc_msg_t))
    {
        char tmpStr[256] = {0};
        _snprintf_s(tmpStr, 256, _T("process %u send reg req hdr to config server failed, error %d"),
			_getpid(), WSAGetLastError());
        MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }
    ret = sock_write_timeout(sockfd, (char*)&req_body, sizeof(reg_req_body_t), -1);
    if (ret != sizeof(reg_req_body_t))
    {
        char tmpStr[256] = {0};
        _snprintf_s(tmpStr, 256, _T("process %u send reg req body to config server failed, error %d"), 
			_getpid(), WSAGetLastError());
        MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }

    ret = sock_read_timeout(sockfd, (char*)&msg, sizeof(proc_msg_t), -1);
    if (ret != sizeof(proc_msg_t))
    {
        char tmpStr[256] = {0};
        _snprintf_s(tmpStr, 256, _T("process %u read reg resp hdr from config server failed, error %d"), 
			_getpid(), WSAGetLastError());
        MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }

    if (msg.type != MSG_REG_RESP)
    {
        char tmpStr[256] = {0};
        _snprintf_s(tmpStr, 256, _T("process %u read reg resp from config server failed, type %d"), _getpid(), msg.type);
        MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }

    ret = sock_read_timeout(sockfd, (char*)resp_data, sizeof(reg_resp_body_t), -1);
    if (ret != sizeof(reg_resp_body_t))
    {
        char tmpStr[256] = {0};
        _snprintf_s(tmpStr, 256, _T("process %u read reg resp body from config server failed, error %d"), _getpid(), WSAGetLastError());
        MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }

    sock_close(sockfd);

	_LOG_INFO("register ok");
    return 0;
}

int proc_send_unreg_req()
{
    int ret = 0;
    int sockfd = sock_connect("127.0.0.1", DEF_PROC_COMM_SRV_PORT);
    if (-1 == sockfd)
    {
        //char tmpStr[256] = {0};
        //_snprintf_s(tmpStr, 256, _T("connect to config server %d, failed, error %d"), DEF_PROC_COMM_SRV_PORT, WSAGetLastError());
        //MessageBox(NULL, tmpStr, _T("Error"), MB_OK);
        return -1;
    }

    proc_msg_t msg;
    msg.type = MSG_UNREG_REQ;
    msg.len = sizeof(unreg_req_body_t);

    unreg_req_body_t req_body;
    req_body.proc_id = g_cur_proc_pid;

    ret = sock_write_timeout(sockfd, (char*)&msg, sizeof(proc_msg_t), -1);
    if (ret != sizeof(proc_msg_t))
    {
        //char tmpStr[256] = {0};
        //_snprintf_s(tmpStr, 256, _T("send unreg req hdr to config server failed, error %d"), WSAGetLastError());
        //MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }
    ret = sock_write_timeout(sockfd, (char*)&req_body, sizeof(reg_req_body_t), -1);
    if (ret != sizeof(reg_req_body_t))
    {
        // char tmpStr[256] = {0};
        // _snprintf_s(tmpStr, 256, _T("send unreg req body to config server failed, error %d"), WSAGetLastError());
        // MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }
    sock_close(sockfd);

	_LOG_INFO("unregister ok");
    return 0;
}

int proc_send_log(char *log_buf, int log_len)
{
    int ret = 0;
    int sockfd = sock_connect("127.0.0.1", DEF_PROC_COMM_SRV_PORT);
    if (-1 == sockfd)
    {
        // char tmpStr[256] = {0};
        // _snprintf_s(tmpStr, 256, _T("connect to config server failed, error %d"), WSAGetLastError());
        // MessageBox(NULL, tmpStr, _T("Error"), MB_OK);
        return -1;
    }

    proc_msg_t msg;
    msg.type = MSG_LOG;
    msg.len = log_len + (uint16_t)sizeof(g_cur_proc_pid);

    ret = sock_write_timeout(sockfd, (char*)&msg, sizeof(proc_msg_t), -1);
    if (ret != sizeof(proc_msg_t))
    {
        // char tmpStr[256] = {0};
        // _snprintf_s(tmpStr, 256, _T("send log hdr to config server failed, error %d"), WSAGetLastError());
        // MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }

    ret = sock_write_timeout(sockfd, (char*)&g_cur_proc_pid, sizeof(g_cur_proc_pid), -1);
    if (ret != sizeof(g_cur_proc_pid))
    {
        // char tmpStr[256] = {0};
        // _snprintf_s(tmpStr, 256, _T("send log body to config server failed, error %d"), WSAGetLastError());
        // MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }

    ret = sock_write_timeout(sockfd, log_buf, log_len, -1);
    if (ret != log_len)
    {
        // char tmpStr[256] = {0};
        // _snprintf_s(tmpStr, 256, _T("send log body to config server failed, error %d"), WSAGetLastError());
        // MessageBox(NULL, tmpStr, _T("Error"), MB_OK);

        sock_close(sockfd);
        return -1;
    }

    sock_close(sockfd);
    return 0;
}

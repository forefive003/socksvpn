
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "netpool.h"
#include "CJobIo.h"
#include "CNetPoll.h"

void CListenJob::set_read_callback(void *callback)
{
	m_accept_func = (accept_hdl_func)callback;
}	

void CListenJob::read_evt_handle()
{
	BOOL acptEnd = false;
	while(!acptEnd)
	{
		int conn_fd = 0;
		struct sockaddr childaddr;
		int len = 0;

		conn_fd = accept(m_fd, &childaddr, (socklen_t*)&len);
		if (conn_fd == -1)
		{
#ifdef _WIN32
			DWORD dwError = WSAGetLastError();
			if (dwError == WSAEINTR || dwError == WSAEWOULDBLOCK)
			{
				acptEnd = true;
				break;
			}
#else
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				acptEnd = true;
				break;
			}
			else if (errno == EINTR)
			{
				_LOG_WARN("EINTR occured, continue");
				continue;
			}
			else
			{
				char err_buf[64] = {0};
				_LOG_ERROR("accept failed [%s]", str_error_s(err_buf, 32, errno));
			}
#endif
		}
		else if (conn_fd == 0)
		{
			_LOG_ERROR("accept return 0, maybe closed.");
			break;
		}

		m_accept_func(conn_fd, m_param1);
	}
}

void CWrIoJob::set_write_callback(void *callback)
{
	m_write_func = (write_hdl_func)callback;
}


void CWrIoJob::write_evt_handle()
{
	_LOG_DEBUG("fd %d write event coming", m_fd);
	if (m_is_want_delete)
	{
		_LOG_INFO("fd %d is deleting, not write.", m_fd);
		return;
	}

	this->lock();
	g_NetPoll->del_io_writing_evt(this);
	this->del_write_io_event();
	this->unlock();
	
	m_write_func(m_fd, m_param1);
}


void CTcpJob::set_read_callback(void *callback)
{
	m_read_func = (read_hdl_func)callback;
}

void CTcpJob::read_evt_handle()
{
	int recvLen = 0;
	BOOL recvEnd = false;

	while(!recvEnd)
	{
		if (m_is_want_delete)
		{
			_LOG_INFO("fd %d is deleting, not read.", m_fd);
			break;
		}

		memset(m_cache_buf, 0, m_cache_len);
		
		recvLen = recv(m_fd, m_cache_buf, m_cache_len, 0);
		if (recvLen < 0)
		{
#ifdef _WIN32
			DWORD dwError = WSAGetLastError();
			if (dwError == WSAEINTR || dwError == WSAEWOULDBLOCK)
			{
				recvEnd = true;
				break;
			}

			char err_buf[64] = {0};
			_LOG_ERROR("read failed [%d]", dwError);
#else
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				recvEnd = true;
				break;
			}
			else if (errno == EINTR)
			{
				_LOG_WARN("EINTR occured, continue");
				continue;
			}
			else
			{
				char err_buf[64] = {0};
				_LOG_ERROR("read failed [%s]", str_error_s(err_buf, 32, errno));
			}
#endif

			recvLen = 0;
			recvEnd = true;
		}
		else if (recvLen == 0)
		{
			/*peer closed*/
			recvEnd = true;
		}

		m_read_func(m_fd, m_param1, m_cache_buf, recvLen);
	}
}


void CUdpJob::set_read_callback(void *callback)
{
	m_read_func = (udp_read_hdl_func)callback;
}

void CUdpJob::read_evt_handle()
{
	int recvLen = 0;
	BOOL recvEnd = false;

	while(!recvEnd)
	{
		if (m_is_want_delete)
		{
			_LOG_INFO("fd %d is deleting, not read.", m_fd);
			break;
		}

		memset(m_cache_buf, 0, m_cache_len);
		
		struct sockaddr cliAddrTmp;
		int clientAddrLen = sizeof(cliAddrTmp);
		memset((void*)&cliAddrTmp, 0, sizeof(cliAddrTmp));

		recvLen = recvfrom(m_fd, m_cache_buf, m_cache_len, 0, &cliAddrTmp, (socklen_t*)&clientAddrLen);
		if (recvLen < 0)
		{
#ifdef _WIN32
			DWORD dwError = WSAGetLastError();
			if (dwError == WSAEINTR || dwError == WSAEWOULDBLOCK)
			{
				recvEnd = true;
				break;
			}

			char err_buf[64] = {0};
			_LOG_ERROR("read failed [%d]", dwError);
#else
			if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
			{
				recvEnd = true;
				break;
			}

			char err_buf[64] = {0};
			_LOG_ERROR("read failed [%s]", str_error_s(err_buf, 32, errno));
#endif

			recvLen = 0;
			recvEnd = true;
		}
		else if (recvLen == 0)
		{
			/*peer closed*/
			recvEnd = true;
		}

		m_read_func(m_fd, m_param1, &cliAddrTmp, m_cache_buf, recvLen);
	}
}

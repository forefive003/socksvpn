#include "common.h"
#include "procCommServ.h"
#include "proxyConfig.h"
#include "clientMain.h"
#include "socketwrap.h"
#include "procMgr.h"

const char* g_comm_msg_desc[] =
{
    "REG-REQ",
    "REG_RESP",
    "UNREG_REQ",
    "LOG-REQ"
};

static CProcMsgServer *g_msgServer;

int CProcMsgServer::accept_handle(int conn_fd, uint32_t client_ip, uint16_t client_port)
{
    int ret = 0;
    proc_msg_t msg;

    /*read msg from conn_fd*/
    ret = sock_read_timeout(conn_fd, (char*)&msg, sizeof(proc_msg_t), -1);
    if (ret != sizeof(proc_msg_t))
    {
        _LOG_ERROR(_T("read msg failed, ret %d"), ret);
        sock_close(conn_fd);
        return -1;
    }

    if (msg.type >= MSG_MAX)
    {
        _LOG_ERROR(_T("invalid msg type %d"), msg.type);
        sock_close(conn_fd);
        return -1;
    }

    proxy_cfg_t* cfginfo = NULL;
    reg_req_body_t reg_req_body;
    reg_resp_body_t reg_resp_body;
    unreg_req_body_t unreg_req_body;

    _LOG_INFO(_T("recv msg, type %s"), g_comm_msg_desc[msg.type]);

    switch(msg.type)
    {
        case MSG_REG_REQ:
            ret = sock_read_timeout(conn_fd, (char*)&reg_req_body, sizeof(reg_req_body_t), -1);
            if (ret != sizeof(reg_req_body_t))
            {
                _LOG_ERROR(_T("read reg req msg failed, ret %d"), ret);
                sock_close(conn_fd);
                return -1;
            }

            cfginfo = proxy_cfg_get();
            reg_resp_body.local_port = cfginfo->local_port;

            msg.type = MSG_REG_RESP;
            msg.len = sizeof(reg_resp_body_t);
            ret = sock_write_timeout(conn_fd, (char*)&msg, sizeof(msg), -1);
            if (ret != sizeof(msg))
            {
                _LOG_ERROR(_T("send reg response hdr msg failed, ret %d"), ret);
                sock_close(conn_fd);
                return -1;
            }

            ret = sock_write_timeout(conn_fd, (char*)&reg_resp_body, sizeof(reg_resp_body_t), -1);
            if (ret != sizeof(reg_resp_body_t))
            {
                _LOG_ERROR(_T("send reg response msg failed, ret %d"), ret);
                sock_close(conn_fd);
                return -1;
            }
            sock_close(conn_fd);

            proxy_proc_set_registered(reg_req_body.proc_id);
            break;
        case MSG_UNREG_REQ:
            ret = sock_read_timeout(conn_fd, (char*)&unreg_req_body, sizeof(unreg_req_body_t), -1);
            if (ret != sizeof(unreg_req_body_t))
            {
                _LOG_ERROR(_T("read unreg req msg failed, ret %d"), ret);
                sock_close(conn_fd);
                return -1;
            }
            proxy_proc_set_uninjected(unreg_req_body.proc_id);
            break;

        case MSG_LOG:
        {
            DWORD proc_id;
            char log_buf[512 + 1] = {0};
            if (msg.len <= sizeof(proc_id))
            {
                _LOG_ERROR(_T("read log req msg failed, len invalid %d"), msg.len);
                sock_close(conn_fd);
                return -1;
            }

            msg.len -= sizeof(proc_id);
            if (msg.len >= 512)
            {
                msg.len = 512;
            }
            
            ret = sock_read_timeout(conn_fd, (char*)&proc_id, sizeof(proc_id), -1);
            if (ret != sizeof(proc_id))
            {
                _LOG_ERROR(_T("read proc id from log req msg failed, ret %d"), ret);
                sock_close(conn_fd);
                return -1;
            }

            ret = sock_read_timeout(conn_fd, (char*)log_buf, msg.len, -1);
            if (ret != msg.len)
            {
                _LOG_ERROR(_T("read log req msg failed, ret %d"), ret);
                sock_close(conn_fd);
                return -1;
            }
			log_buf[ret] = 0;
				
			///TODO: notice, should be llu, or oops
            _LOG_INFO(_T("###LOG-PROC-%u###:%s"), proc_id, log_buf);
        }
        break;
        default:
            _LOG_ERROR(_T("read msg, invalid type %d"), msg.type);
            sock_close(conn_fd);
            return -1;
            break;
    }

    sock_close(conn_fd);
    return 0;
}

int proc_comm_init()
{
	proxy_cfg_t *cfginfo = proxy_cfg_get();
		g_msgServer = new CProcMsgServer(cfginfo->config_port);
    if(0 != g_msgServer->init())
    {
        return -1;
    }

    return 0;
}

void proc_comm_free()
{
    return;
}


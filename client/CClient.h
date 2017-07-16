#ifndef _CLI_CLIENT_H
#define _CLI_CLIENT_H

class CClient : public CBaseClient
{
public:
	CClient(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner) : CBaseClient(ipaddr, port, fd, owner)
    {
        m_status = CLI_INIT;
        m_request_time = 0;
    }
	virtual ~CClient()
	{
	}

private:
	int recv_handle(char *buf, int buf_len);

public:
	int get_client_status();
	void set_client_status(CLI_STATUS_E status);

	void auth_result_handle(BOOL result);
    void connect_result_handle(BOOL result);

    int send_remote_close_msg()
    {
        /*nothing to send, will close client in free_handle*/
        return 0;
    }
	int send_data_msg(char *buf, int buf_len);
private:
	int parse_handshake(char *buf, int buf_len);
	int parse_connect(char *buf, int buf_len);

private:
	CLI_STATUS_E m_status;
	uint64_t m_request_time;
};

#endif


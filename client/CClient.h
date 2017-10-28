#ifndef _CLI_CLIENT_H
#define _CLI_CLIENT_H


#define PROXY_MSG_MAGIC 0xfeebbeef

typedef struct 
{
	/* data */
	uint32_t magic;
	uint32_t reserved1;
	uint64_t proc_id;
	uint32_t remote_ipaddr;
	uint16_t remote_port;
	uint16_t reserved2;
}register_req_t;

typedef struct
{
	uint32_t result;
}register_resp_t;

class CClient : public CBaseClient
{
public:
	CClient(uint32_t ipaddr, uint16_t port, int fd, CBaseConnection *owner) : CBaseClient(ipaddr, port, fd, owner)
    {
    	m_is_standard_socks = FALSE;
    	
        m_status = SOCKS_INIT;
        m_proc_id = 0;

        m_real_remote_ipaddr = 0;
        m_real_remote_port = 0;
    }
	virtual ~CClient()
	{
	}

public:
	void free(); 

private:
	int register_req_handle(char *buf, int buf_len);
	int socks_proto4_handle(char *buf, int buf_len);
	int socks_proto5_handle(char *buf, int buf_len);
	void auth_result_handle(BOOL result);

	int socks_parse_handshake(char *buf, int buf_len);
	int socks_parse_connect(char *buf, int buf_len);

	int recv_handle(char *buf, int buf_len);

public:   
	void set_real_server(uint32_t real_serv, uint16_t real_port);
	void get_real_server(uint32_t *real_serv, uint16_t *real_port);
	void set_real_domain(char *domain_name);
	void get_real_domain(char *domain_name);

	int get_client_status();
	void set_client_status(SOCKS_STATUS_E status); 

	void connect_result_handle(BOOL result, int remote_ipaddr);  

    int send_remote_close_msg()
    {
        /*nothing to send, will close client in free_handle*/
        return 0;
    }
	int send_data_msg(char *buf, int buf_len)
	{
		return send_data(buf, buf_len);
	}

private:
	int m_socks_proto_version; /*4 or 5*/

public:
	SOCKS_STATUS_E m_status;
	uint64_t m_proc_id;
	BOOL m_is_standard_socks; /*是否来自标准的socks客户端*/

	uint64_t m_request_time;

	uint32_t m_real_remote_ipaddr; /*真实的远端服务器ip*/
    uint16_t m_real_remote_port; /*真实的远端服务器端口*/
    char m_remote_domain[HOST_DOMAIN_LEN + 1];
};

#endif


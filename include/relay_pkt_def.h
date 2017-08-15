#ifndef _RELAY_PACKET_H
#define _RELAY_PACKET_H

enum pkt_type_e{
	PKT_C2R = 1, /*client to relay*/
	PKT_R2S, /*relay to server*/
	PKT_S2R, /*server to relay*/
	PKT_R2C, /*relay to client*/

	PKT_S2R_REG, /*server to relay, for register and keepalive*/

	PKT_TYPE_MAX
};

typedef struct _pkt_hdr_
{
	uint16_t pkt_type;
	uint16_t pkt_len;
}PKT_HDR_T;

enum pkt_sub_type_e{
	C2R_AUTH = 1,
	C2R_CONNECT = 2,
	C2R_DATA = 3,

	R2C_AUTH_RESULT = 4,
	R2C_CONNECT_RESULT = 5,
	R2C_DATA = 6,

	R2S_CLIENT_CONNECT = 7,
	R2S_DATA = 8,

	S2R_CONNECT_RESULT = 9,
	S2R_DATA = 10,

	CLIENT_CLOSED = 11,
	REMOTE_CLOSED = 12,

	PKT_SUB_TYPE_MAX
};

typedef struct _pkt_c2r_
{
	uint32_t server_ip;
	uint32_t client_ip; /*not used*/
	uint16_t server_port;
	uint16_t client_port;
	uint16_t sub_type;
	uint16_t reserved;
}PKT_C2R_HDR_T;


typedef struct _pkt_r2s_
{
	uint32_t client_pub_ip;
	uint32_t client_inner_ip;
	uint16_t client_pub_port;
	uint16_t client_inner_port;
	uint16_t sub_type;
}PKT_R2S_HDR_T;


typedef struct _pkt_s2r_
{
	uint32_t client_pub_ip;
	uint32_t client_inner_ip;
	uint16_t client_pub_port;
	uint16_t client_inner_port;
	uint16_t sub_type;
}PKT_S2R_HDR_T;


typedef struct _pkt_r2c_
{
	uint32_t server_ip;
	uint32_t client_ip;	 /*not used*/
	uint16_t server_port;
	uint16_t client_port;
	uint16_t sub_type;
	uint16_t reserved;
}PKT_R2C_HDR_T;

typedef struct _server_reg_pkt_
{
	uint32_t local_ip;
	uint16_t local_port;
	uint16_t is_keepalive;
	char sn[MAX_SN_LEN];
}PKT_SRV_REG_T;

#endif


#ifndef _COMMON_DEF_H
#define _COMMON_DEF_H

#define MAX_USERNAME_LEN 64
#define MAX_PASSWD_LEN 64

#define DEF_CLIENT_PORT 22222
#define DEF_RELAY_CLI_PORT 22223
#define DEF_RELAY_SRV_PORT 22225
#define DEF_CONFIG_SRV_PORT 22226

#define  CONNECT_FAILED_FLAG 0xFF

#define CLIENT_MEM_NODE_CNT  2048
#define SERVER_MEM_NODE_CNT  8192
#define RELAY_MEM_NODE_CNT  40960

typedef enum 
{
	CLI_INIT = 1,
	CLI_AUTHING,
	CLI_CONNECTING,
	CLI_CONNECTED
}CLI_STATUS_E;

#define CLIENT_CONVERT(baseclient)  ((CClient*)(baseclient))
#define REMOTE_CONVERT(baseremote)  ((CRemote*)(baseremote))



#define FOR_TEST 1

#ifdef FOR_TEST
typedef struct _encry_key_s {
	size_t len;
	uint8_t key[0];
}encry_key_t;

typedef enum _encry_method {
	NO_ENCRYPT = 0,
	XOR_METHOD = 1,
	RC4_METHOD,
}encry_method_e;


typedef struct _encryptor {
	encry_method_e enc_method;
	//union {
	//	struct xor_encryptor xor_enc;
	//	struct rc4_encryptor rc4_enc;
	//};
}encryptor_t;
#endif

#endif

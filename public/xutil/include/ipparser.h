
#ifndef IP_PARSER_H_
#define IP_PARSER_H_

#include <sys/socket.h>
#include <netinet/in.h>

 
#define IP_DESC_LEN  INET6_ADDRSTRLEN

#define HOST_IP_LEN  IP_DESC_LEN
#define HOST_DOMAIN_LEN 256

typedef struct{
	struct sockaddr_storage startAddr;
	struct sockaddr_storage endAddr;
}IP_RANGE_KEY_T;

unsigned long long ntohll(unsigned long long val);
unsigned long long htonll(unsigned long long val);
bool host_addr_check(char *hostname);


char *util_trim(char *src, char find);

int util_str_to_ip(char* ipstr, struct sockaddr_storage *ipaddr);
char* util_ip_to_str(struct sockaddr_storage *ipaddr, char *ipstr);

int util_ip_cmp(struct sockaddr_storage *ipaddr1,
					struct sockaddr_storage *ipaddr2);

void util_ip_hton(struct sockaddr_storage *ipaddr);
void util_ip_ntoh(struct sockaddr_storage *ipaddr);


int util_parse_ip_scope(char* ipstr, IP_RANGE_KEY_T* ipRange);
int util_parse_ip_ranges(char* ipRangeStr,
				IP_RANGE_KEY_T* ipRanges, int *ipRangesCnt,
				int maxCnt);

#endif /* IP_PARSER_H_ */

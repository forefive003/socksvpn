#include <arpa/inet.h>

#include "commtype.h"
#include "ipparser.h"

unsigned long long ntohll(unsigned long long val)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        return (((unsigned long long )htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
        return val;
    }
}

unsigned long long htonll(unsigned long long val)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        return (((unsigned long long )htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
        return val;
    }
}


bool host_addr_check(char *hostname)
{
	int count = 0;
	char* p = hostname;

	while(0 != *p)
	{
	   if(*p =='.')
		if((p+1) != NULL && *(p+1)!= '.')
			count++;
	   p++;
	}

	if(count != 3)
	{
		return FALSE;
	}

	return TRUE;
}

char *util_trim(char *src, char find)
{
	int i = 0;
	char *begin = src;
	while(src[i] != '\0')
	{
		if(src[i] != find)
		{
			break;
		}
		else
		{
			begin++;
		}
		i++;
	}

	for(i = strlen(src)-1; i >= 0; i--)
	{
		if(src[i] != find)
		{
			break;
		}
		else
		{
			src[i] = '\0';
		}
	}

	return begin;
}

int util_str_to_ip(char* ipstr, struct sockaddr_storage *ipaddr)
{
///TODO ipv6

	if (FALSE == host_addr_check(ipstr))
	{
		fprintf(stderr, "invalid host addr %s.", ipstr);
		return -1;
	}
	struct in_addr tempaddr;
	if(0 == inet_pton(AF_INET, ipstr, (void *)&tempaddr))
	{
		fprintf(stderr, "ipdesc %s invalid.", ipstr);
		return -1;
	}

	ipaddr->ss_family = AF_INET;
	((struct sockaddr_in *)ipaddr)->sin_addr.s_addr = tempaddr.s_addr;
	return 0;
}

char* util_ip_to_str(struct sockaddr_storage *ipaddr, char *ipstr)
{
	if (ipaddr->ss_family == AF_INET)
	{
		struct in_addr tempaddr;
		tempaddr.s_addr = ((struct sockaddr_in *)ipaddr)->sin_addr.s_addr;

		if (inet_ntop(AF_INET, (void*)&tempaddr, ipstr, IP_DESC_LEN) == NULL)
		{
			fprintf(stderr, "inet_ntop failed.\n");
			return NULL;
		}
	}
	else if (ipaddr->ss_family == AF_INET6)
	{
///TODO:IPV6
	}

	return ipstr;
}

void util_ip_hton(struct sockaddr_storage *ipaddr)
{
	if (ipaddr->ss_family == AF_INET)
	{
		((struct sockaddr_in *)ipaddr)->sin_addr.s_addr
				= htonl(((struct sockaddr_in *)ipaddr)->sin_addr.s_addr);
	}

///TODO IPV6
}

void util_ip_ntoh(struct sockaddr_storage *ipaddr)
{
	util_ip_hton(ipaddr);
}

int util_ip_cmp(struct sockaddr_storage *ipaddr1,
			struct sockaddr_storage *ipaddr2)
{
	if (ipaddr1->ss_family != ipaddr2->ss_family)
	{
		return -1;
	}

	if (ipaddr1->ss_family == AF_INET)
	{
		return (ntohl(((struct sockaddr_in *)ipaddr1)->sin_addr.s_addr)
				- ntohl(((struct sockaddr_in *)ipaddr2)->sin_addr.s_addr));
	}

///TODO IPV6
	return -1;
}



int util_parse_ip_scope(char* ipstr, IP_RANGE_KEY_T* ipRange)
{
	char *p, *pch;
    int len;
    char str[IP_DESC_LEN] = {0};

    struct sockaddr_storage beginIp, endIp;
    memset(&beginIp, 0, sizeof(beginIp));
    memset(&endIp, 0, sizeof(endIp));

	if ((p = strpbrk(ipstr, "/")) != NULL)
	{
		*p = '\0';
		p++;

		struct sockaddr_storage ipnet;
		int32_t mask = 0;

		if (util_str_to_ip(ipstr, &ipnet) != 0)
		{
			return -1;
		}

		memcpy((void*)&beginIp, (void*)&ipnet, sizeof(ipnet));
		memcpy((void*)&endIp, (void*)&ipnet, sizeof(ipnet));

		mask = atoi(p);

		/*IPV4*/
		unsigned int t = 0;
		t = htonl(0xffffffff << ( 32- mask));
		((struct sockaddr_in *)&beginIp)->sin_addr.s_addr
				= ((struct sockaddr_in *)&ipnet)->sin_addr.s_addr & t;

		t = 0xffffffff - t;
		((struct sockaddr_in *)&endIp)->sin_addr.s_addr
						= ((struct sockaddr_in *)&ipnet)->sin_addr.s_addr | t;

///TODO IPV6
	}
	else if ((p = strpbrk(ipstr, "-")) != NULL)
	{
		*p = '\0';
		p++;

		if ((pch = strrchr(ipstr, '.')) != NULL)
		{
			if (util_str_to_ip(ipstr, &beginIp) != 0)
			{
				return -1;
			}

			if ((strchr(p, '.') != NULL))
			{
				/*192.168.1.1-192.168.1.60*/
				if (util_str_to_ip(p, &endIp) != 0)
				{
					return -1;
				}
			}
			else
			{
	        	/*192.168.1.1-60*/
	            len = pch - ipstr + 1;
	            strncpy(str, ipstr, len);
	            strncpy(str + len, p, strlen(p));
	            p = str;

	            if (util_str_to_ip(p, &endIp) != 0)
				{
					return -1;
				}
			}
		}
		else
		{
			return -1;
		}
	}
	else
	{
		if (util_str_to_ip(ipstr, &beginIp) != 0)
		{
			return -1;
		}
		endIp = beginIp;
	}

	memcpy((void*)&ipRange->startAddr, (void*)&beginIp, sizeof(beginIp));
	memcpy((void*)&ipRange->endAddr, (void*)&endIp, sizeof(endIp));
	return 0;
}

int util_parse_ip_ranges(char* ipRangeStr,
				IP_RANGE_KEY_T* ipRanges, int *ipRangesCnt,
				int maxCnt)
{
	char *tmpstr = NULL;

	/*先去前后空格*/
	ipRangeStr = util_trim(ipRangeStr, ' ');

	tmpstr = strtok(ipRangeStr, ",");
	if (NULL == tmpstr)
	{
		fprintf(stderr, "no ipnet in params.");
		return -1;
	}

	uint16_t ipIndex = 0;
	while(tmpstr != NULL)
	{
		if (ipIndex >= maxCnt)
		{
			fprintf(stderr, "too many ipnet in params.");
			return -1;
		}

		if (util_parse_ip_scope(tmpstr, &ipRanges[ipIndex]))
		{
			fprintf(stderr, "parse ipnet failed, %s.", tmpstr);
			return -1;
		}

		ipIndex++;
		tmpstr = strtok(NULL, ",");
	}

	*ipRangesCnt = ipIndex;
	return 0;
}


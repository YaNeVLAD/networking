#include <common/net/c/conversions.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

uint16_t u16_to_network(const uint16_t value)
{
	return htons(value);
}

uint16_t u16_to_host(const uint16_t value)
{
	return ntohs(value);
}

uint32_t u32_to_network(const uint32_t value)
{
	return htonl(value);
}

uint32_t u32_to_host(const uint32_t value)
{
	return ntohl(value);
}
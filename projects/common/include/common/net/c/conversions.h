#ifndef NETWORKING_UTILS_H
#define NETWORKING_UTILS_H

#include <stdint.h>

#include <common/net/config.h>

NET_C_START

uint16_t u16_to_network(uint16_t value);
uint16_t u16_to_host(uint16_t value);

uint32_t u32_to_network(uint32_t value);
uint32_t u32_to_host(uint32_t value);

NET_C_END

#endif // NETWORKING_UTILS_H
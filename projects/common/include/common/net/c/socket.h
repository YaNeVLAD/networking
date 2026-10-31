#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <stddef.h>

#include <common/net/config.h>

NET_C_START

typedef intptr_t socket_t;
#define NET_INVALID_SOCKET ((socket_t)(-1))
#define NET_SOCKET_ERROR ((socket_t)(-1))

NET_C_END

#endif
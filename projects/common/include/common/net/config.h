#ifndef NET_CONFIG_H
#define NET_CONFIG_H

#ifdef __cplusplus

#define NET_C_START extern "C" {
#define NET_C_END }

#else

#define NET_C_START
#define NET_C_END

#endif

#ifdef _WIN32

#define NET_PLATFORM_WINDOWS

#else

#define NET_PLATFORM_POSIX

#endif

#endif // NET_CONFIG_H

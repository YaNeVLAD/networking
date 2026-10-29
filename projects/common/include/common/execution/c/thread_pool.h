#ifndef NETWORKING_THREAD_POOL_H
#define NETWORKING_THREAD_POOL_H

#include <stddef.h>

#include <common/net/config.h>

NET_C_START

typedef struct thread_pool thread_pool_t;
typedef void (*thread_pool_task_fn)(void* arg);

thread_pool_t* thread_pool_create(size_t num_threads);
int thread_pool_enqueue(thread_pool_t* pool, thread_pool_task_fn function, void* arg);
void thread_pool_destroy(thread_pool_t* pool);

NET_C_END

#endif // NETWORKING_THREAD_POOL_H

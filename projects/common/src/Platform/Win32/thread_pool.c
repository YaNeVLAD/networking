#include <common/execution/c/thread_pool.h>

#include <stdlib.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef struct task
{
	thread_pool_task_fn function;
	void* arg;
	struct task* next;
} task_t;

struct thread_pool
{
	HANDLE* threads;
	size_t num_threads;

	task_t* head;
	task_t* tail;

	CRITICAL_SECTION lock;
	CONDITION_VARIABLE cv;

	int stop;
};

// ReSharper disable once CppParameterMayBeConst
static DWORD WINAPI worker_thread(LPVOID arg)
{
	thread_pool_t* pool = arg;

	while (1)
	{
		EnterCriticalSection(&pool->lock);

		while (pool->head == NULL && !pool->stop)
		{
			SleepConditionVariableCS(&pool->cv, &pool->lock, INFINITE);
		}

		if (pool->stop && pool->head == NULL)
		{
			LeaveCriticalSection(&pool->lock);
			break;
		}

		task_t* task = pool->head;
		if (task)
		{
			pool->head = task->next;
			if (pool->head == NULL)
			{
				pool->tail = NULL;
			}
		}

		LeaveCriticalSection(&pool->lock);

		if (task)
		{
			task->function(task->arg);
			free(task);
		}
	}

	return 0;
}

thread_pool_t* thread_pool_create(const size_t num_threads)
{
	if (num_threads == 0)
	{
		return NULL;
	}

	thread_pool_t* pool = malloc(sizeof(thread_pool_t));
	if (!pool)
	{
		return NULL;
	}

	pool->num_threads = num_threads;
	pool->head = NULL;
	pool->tail = NULL;
	pool->stop = 0;

	InitializeCriticalSection(&pool->lock);
	InitializeConditionVariable(&pool->cv);

	pool->threads = (HANDLE*)malloc(sizeof(HANDLE) * num_threads);
	if (!pool->threads)
	{
		DeleteCriticalSection(&pool->lock);
		free(pool->threads);
		free(pool);

		return NULL;
	}

	size_t created_threads = 0;
	for (; created_threads < num_threads; ++created_threads)
	{
		pool->threads[created_threads] = CreateThread(NULL, 0, worker_thread, pool, 0, NULL);

		if (!pool->threads[created_threads])
		{
			break;
		}
	}

	if (created_threads != num_threads)
	{
		EnterCriticalSection(&pool->lock);
		pool->stop = 1;
		WakeAllConditionVariable(&pool->cv);
		LeaveCriticalSection(&pool->lock);

		for (size_t i = 0; i < created_threads; ++i)
		{
			WaitForSingleObject(pool->threads[i], INFINITE);
			CloseHandle(pool->threads[i]);
		}

		free(pool->threads);
		DeleteCriticalSection(&pool->lock);
		free(pool);

		return NULL;
	}

	return pool;
}

int thread_pool_enqueue(thread_pool_t* pool, const thread_pool_task_fn function, void* arg)
{
	if (!pool || !function)
	{
		return -1;
	}

	task_t* new_task = malloc(sizeof(task_t));
	if (!new_task)
	{
		return -1;
	}

	new_task->function = function;
	new_task->arg = arg;
	new_task->next = NULL;

	EnterCriticalSection(&pool->lock);

	if (pool->stop)
	{
		LeaveCriticalSection(&pool->lock);
		free(new_task);

		return -1;
	}

	if (pool->tail == NULL)
	{
		pool->head = new_task;
		pool->tail = new_task;
	}
	else
	{
		pool->tail->next = new_task;
		pool->tail = new_task;
	}

	WakeConditionVariable(&pool->cv);
	LeaveCriticalSection(&pool->lock);

	return 0;
}

void thread_pool_destroy(thread_pool_t* pool)
{
	if (!pool)
	{
		return;
	}

	EnterCriticalSection(&pool->lock);
	pool->stop = 1;
	WakeAllConditionVariable(&pool->cv);
	LeaveCriticalSection(&pool->lock);

	for (size_t i = 0; i < pool->num_threads; ++i)
	{
		WaitForSingleObject(pool->threads[i], INFINITE);
		CloseHandle(pool->threads[i]);
	}

	task_t* current = pool->head;
	while (current)
	{
		task_t* next = current->next;
		free(current);
		current = next;
	}

	DeleteCriticalSection(&pool->lock);
	free(pool->threads);
	free(pool);
}
#include <common/execution/ThreadPool.hpp>

using namespace execution;

ThreadPool::ThreadPool()
	: ThreadPool(std::thread::hardware_concurrency())
{
}

ThreadPool::ThreadPool(const size_t num_threads)
	: m_pool(thread_pool_create(num_threads))
{
	if (!m_pool)
	{
		throw std::runtime_error("Failed to create thread pool");
	}
}

ThreadPool::~ThreadPool()
{
	thread_pool_destroy(m_pool);
}

void ThreadPool::c_task_callback(void* arg) noexcept
{
	const std::unique_ptr<TaskWrapper> wrapper(static_cast<TaskWrapper*>(arg));
	try
	{
		wrapper->function();
	}
	catch (...)
	{
		// Exceptions must not escape through the C callback.
	}
}
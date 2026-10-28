#pragma once

#include <common/execution/c/thread_pool.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace execution
{

class ThreadPool
{
	struct TaskWrapper
	{
		std::move_only_function<void()> function;
	};

public:
	ThreadPool();

	explicit ThreadPool(size_t num_threads);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	~ThreadPool();

	template <typename Fn>
		requires std::is_constructible_v<std::move_only_function<void()>, Fn>
	void enqueue(Fn&& f)
	{
		auto wrapper = std::make_unique<TaskWrapper>(TaskWrapper{ std::forward<Fn>(f) });
		if (auto* arg = wrapper.release(); thread_pool_enqueue(m_pool, &c_task_callback, arg) != 0)
		{
			delete arg;
			throw std::runtime_error("Failed to enqueue thread pool");
		}
	}

private:
	static void c_task_callback(void* arg) noexcept;

	thread_pool_t* m_pool{};
};

} // namespace execution
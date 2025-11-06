#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <print>
#include <algorithm>
#include <functional>
#include <type_traits>

namespace detail {

	class ThreadPool {
	public:

		ThreadPool(size_t threads_size) : threads_size(threads_size) {
			threads_pool.reserve(threads_size); // reserve raw memory
			for (int i = 0; i < threads_size; ++i)
				threads_pool.emplace_back( [this](int i) {thread_function(i); }, i);
		}

		void shut_down() {
			{
				std::unique_lock<std::mutex> lock(pool_mutex);
				b_shut_down = true;
			}
			condition_variable.notify_all(); // notify waiting threads
		}

		void await_termination() { for (auto& thread : threads_pool) if(thread.joinable()) thread.join(); }

		template <typename Func,typename...Args>
		void add_task(Func&&f,Args&&...args) requires std::is_invocable_v<Func,Args...> {
			{
				std::unique_lock<std::mutex> _(pool_mutex);

				if (b_shut_down) return; // dont allow more tasks to be inserted

					///lambda wrapper around the actuall function to be called
				tasks_queue.push([func = std::forward<Func>(f), ...args = std::forward<Args>(args)]() mutable { func(args...); });
			}
			condition_variable.notify_one();
		}

		~ThreadPool() {
			shut_down();
			await_termination();
		}

	private:

		void thread_function(size_t number) {
			
			std::function<void()> func;
			
			for (;;) {
				{
					std::unique_lock<std::mutex> lock(pool_mutex);

					condition_variable.wait(lock, [&]() { return b_shut_down || !tasks_queue.empty(); });

					if (b_shut_down && tasks_queue.empty()) {
						std::println("Thread {} exiting gracefully", number);
						return;
					}

					func = std::move(tasks_queue.front());
					tasks_queue.pop();
					
				}

				func();

			}
		}

		size_t threads_size{};
		std::vector<std::jthread> threads_pool;
		std::queue <std::function<void()>> tasks_queue;

		std::mutex pool_mutex;
		std::condition_variable condition_variable;
		bool b_shut_down{ false };
	};

}

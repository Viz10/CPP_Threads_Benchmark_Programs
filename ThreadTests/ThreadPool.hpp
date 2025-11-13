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
#include <condition_variable>

namespace detail {

	class ThreadPool {
	public:

		ThreadPool(size_t threads_size) : threads_size(threads_size) {
			threads_pool.reserve(threads_size); // reserve raw memory
			for (int i = 0; i < threads_size; ++i)
				threads_pool.emplace_back([this](std::stop_token stoken, int i) {
				thread_function(i, stoken);
					}, i);
		}

		void shut_down() {

			for (auto& thread : threads_pool) {
				thread.request_stop();
			}

			condition_variable.notify_all(); // notify waiting threads to finish remaining tasks
		}

		void await_termination() { for (auto& thread : threads_pool) if(thread.joinable()) thread.join(); }

		template <typename Func,typename...Args>
		void add_task(Func&&f,Args&&...args) requires std::is_invocable_v<Func,Args...> {
			
			{
			std::scoped_lock<std::mutex> _(pool_mutex); 
			tasks_queue.push([func = std::forward<Func>(f), ...args = std::forward<Args>(args)]() mutable { func(args...); });
			///lambda wrapper around the actuall function to be called
			}
		
			condition_variable.notify_one();
		}

		// jthread destructors automatically request_stop and notify
		~ThreadPool() {
			shut_down();
			await_termination(); /// for safety 
		}

	private:

		void thread_function(size_t number, std::stop_token stoken) {
			
		std::function<void()> func;
			
		/// wait for tasks
		for (;;) {
			{
				std::unique_lock<std::mutex> lock(pool_mutex);

				/// wake up when:
				condition_variable.wait(lock,stoken,[&]() { return stoken.stop_requested() || !tasks_queue.empty(); });

				/// stop thread
				if (stoken.stop_requested() && tasks_queue.empty()) {
					std::println("Thread {} exiting gracefully", number);
					return;
				}

				/// get top task
				func = std::move(tasks_queue.front());
				tasks_queue.pop();
					
			}

			/// execute top task
			func();

		}
	}

		size_t threads_size{};
		std::queue<std::function<void()>> tasks_queue;
		std::mutex pool_mutex;
		std::condition_variable_any condition_variable;
		std::vector<std::jthread> threads_pool;
	};

}

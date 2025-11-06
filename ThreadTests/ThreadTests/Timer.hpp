#include <chrono>

namespace detail {
	class Timer {

	public:
		Timer() { start_timer();}
		~Timer() { if (!already_ended) end_timer(); }
		long long end_timer() {
			end = std::chrono::steady_clock::now();
			time_spent = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			already_ended = true;
			return time_spent.count();
		}

	private:
		void start_timer() { start = std::chrono::steady_clock::now(); }
		std::chrono::time_point<std::chrono::steady_clock> start{};
		std::chrono::time_point<std::chrono::steady_clock> end{};
		std::chrono::milliseconds time_spent{};
		bool already_ended = false;
	};
}
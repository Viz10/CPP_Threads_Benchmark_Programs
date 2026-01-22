#include <chrono>
#include <variant>
namespace detail {
	class Timer {

	public:
		Timer() { start_timer();}
		~Timer() { if (!already_ended) end_timer(); }
		auto end_timer() -> std::pair<long long,std::string_view>{
			end = std::chrono::steady_clock::now();
			time_spent = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			already_ended = true;
			
			if (std::get<0>(time_spent).count() == 0) {
				time_spent = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
				return { std::get<1>(time_spent).count(),"ns" };
			}

			return {std::get<0>(time_spent).count(),"ms" };
		}
	
	private:
		void start_timer() { start = std::chrono::steady_clock::now(); }
		std::chrono::time_point<std::chrono::steady_clock> start{};
		std::chrono::time_point<std::chrono::steady_clock> end{};
		std::variant<std::chrono::milliseconds,std::chrono::nanoseconds> time_spent{};
		bool already_ended = false;
	};
}
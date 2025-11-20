#include <type_traits>
#include <iostream>

namespace detail {

	template<typename T>
	concept DefConstructibleOrTrivial =  std::is_default_constructible_v<T>;

	template<DefConstructibleOrTrivial T>
	class Array {

	public:

		template<typename Margins>
		Array(size_t lenght, Margins start_value, Margins end_value) : lenght{ lenght }
		{
			array.resize(lenght);

			std::random_device rd; /// random device
			std::mt19937 rg(rd()); /// random generator

			std::for_each(array.begin(), array.end(),
				[&](auto& element) {
					if constexpr (std::is_floating_point_v<Margins>) {
						std::uniform_real_distribution<Margins> dist(start_value, end_value);
						element = dist(rg);
					}
					else { /// integral type int llu char etc.
						std::uniform_int_distribution<int> dist(start_value, end_value);
						element = static_cast<Margins>(dist(rg));
					}
				});
		}

		template<typename U> requires std::is_constructible_v<T, U>
		Array(std::initializer_list<U> data) : lenght(data.size()) {
			array.resize(lenght);
			std::copy(data.begin(), data.end(), array.begin());
		}

		void print_array() {
			for (const auto& val : array)
				std::print("{} ", val);
		}

		std::vector<T>& get_array() { return array;}

		size_t size() const { return lenght;}
		
	private:

		size_t lenght;
		std::vector<T> array;
	};

	/// Deduction guides
	template<typename U>
	Array(std::initializer_list<U>) -> Array<U>;

	template<typename Margins>
	Array(int , Margins , Margins ) -> Array<Margins>;
}
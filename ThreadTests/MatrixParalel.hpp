#pragma once
#include <iostream>
#include <vector>
#include <print>
#include <algorithm>
#include <functional>
#include <type_traits>

namespace detail {

	template <typename T>
	concept multicable = requires (T n) {
		{ n * n } noexcept -> std::convertible_to<int>;
	};

	template <typename T>
	class Matrix {
	public:

		using data_type = T;
		using reference_type = T&;
		using const_reference_type =const T&;

		template<multicable Bounds>
		Matrix(size_t rows, size_t columns, Bounds begin, Bounds end) :rows(rows), columns(columns) {
			_matrix.resize(rows);
			for (std::vector<T>& line : _matrix) { line.reserve(columns); } /// alocate raw memory

			std::random_device rd;
			std::mt19937 rg(rd());

			std::for_each(_matrix.begin(), _matrix.end(),
				[&](auto& line) {
					if constexpr (std::is_floating_point_v<Bounds>) {
						std::uniform_real_distribution<Bounds> real_dist(begin, end);
						for (int i = 0; i < columns; ++i) line.emplace_back(real_dist(rg));
					}
					else {
						std::uniform_int_distribution<int> dist(begin, end);
						for (int i = 0; i < columns; ++i) line.emplace_back(static_cast<Bounds>(dist(rg)));
					}
				});

		}

		void print_matrix() const {
			for (const auto& row : _matrix) {
				for (const auto& column : row) {
					std::print("{} ", column);
				}
				std::println("");
			}
		}
		reference_type at(size_t row, size_t col) {
			return _matrix[row][col];
		}
		const_reference_type at(size_t row, size_t col) const {
			return _matrix[row][col];
		}

		std::vector<T>& operator[](size_t index) { return _matrix[index]; }
		const std::vector<T>& operator[](size_t index) const { return _matrix[index]; }
		
		  std::vector<T> get_column(size_t index)  { 
			 std::vector<T> result;
			 result.reserve(rows);

			 for (const auto& line : _matrix) { result.emplace_back(line[index]); }

			 return result;
		}

	private:
		size_t rows, columns;
		std::vector<std::vector<T>> _matrix;
	};

	template<multicable Bounds>
	Matrix(size_t, size_t, Bounds, Bounds) -> Matrix<Bounds>;

}
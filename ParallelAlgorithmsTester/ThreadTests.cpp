#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <memory>
#include <random>
#include <print>
#include <cassert>
#include <future>
#include <array>
#include <numbers>
#include <numeric>
#include <filesystem>
#include <fstream>

#include "Array.hpp"
#include "Timer.hpp"
#include "ThreadPool.hpp"
#include "MatrixParalel.hpp"
// 10.000 logging (cout) tasks

/*
Set of benchmark programs to evaluate the performance of a multicore processor.
Set of multithreaded programs + global and thread
execution time measurement. Analysis of execution times when the number of threads varies.
*/

using std::cout;
using std::cin;
using detail::Timer;
using detail::Array;
using detail::ThreadPool;
using detail::Matrix;

template<typename T> requires std::is_default_constructible_v<T>
class accumulate_helper { /// iterator like wrapper
	
	T accumalate{};

	public:
	
	T value() { return accumalate; };
	accumulate_helper& operator++() { return *this; };
	accumulate_helper& operator*()  { return *this; };
	accumulate_helper& operator=(const T& to_be_added) { accumalate += to_be_added; return *this; };

};

template <typename T> 
struct get_vector_underlying_type;

template <typename T>
struct get_vector_underlying_type<std::vector<T>> : std::type_identity<T> {};

template <typename T>
using get_vector_underlying_type_t = typename get_vector_underlying_type<T>::type;

class Tests {

public:
		
	Tests() = default;
	
	void findMinMaxParallel(size_t const nr_threads, size_t _size) {
		
		if (nr_threads <= 0) {
			std::cout << "Nr of threads must be > 0" << "\n";
			return;
		}

		Array array(_size, 10000, 1000000);
		const auto& arr = array.get_array();
		size_t size = arr.size();
		size_t chunk_size = size / nr_threads;

		Timer global;

		std::vector<std::pair<long long, std::string_view>> thread_results(nr_threads + 1); /// for each thread display + function

		int min{ INT_MAX }, max{ INT_MIN };
		std::mutex mtx;
		
		auto func = [&arr, &thread_results, &min, &max,&mtx](int thread_no, int start, int end) {
			Timer timer;

			int local_min=INT_MAX,local_max=INT_MIN;
			
			for (int i = start; i < end; ++i) {
				if (arr[i] < local_min) local_min = arr[i];
				if (arr[i] > local_max) local_max = arr[i];
			}

			/// verify once at the threads end
			{
				std::scoped_lock<std::mutex> _(mtx);
				if (local_min < min) min = local_min;
				if (local_max > max) max = local_max;
			}

			thread_results[thread_no]=timer.end_timer();
		};

		{
			std::vector<std::jthread> threads;
			for (int i = 0; i < nr_threads; ++i) {
				size_t end = (i == nr_threads - 1) ? size : (i + 1) * chunk_size; /// last thread gets end of array
				threads.emplace_back(func, i, i * chunk_size, end);
			}
		} /// RAII join jthread , after this all threads have succesfully computed and written time result


		thread_results[nr_threads] = global.end_timer();

		for (const auto& [first,second] : thread_results) {
			std::println("{} {}", first, second);
		}

	}
	
	void matrixMultiplicationParallel(size_t nr_threads, size_t rowsA, size_t columnsA, size_t rowsB, size_t columnsB) {

		if (columnsA != rowsB) { std::print("Size of line elements for A just be equal to size of column elements for B"); return; }

		Matrix<int> A(rowsA, columnsA, 0, 100); /// element from 0 to 100
		Matrix<int> B(rowsB, columnsB, 0, 100);
		Matrix<int> Result(rowsA, columnsB, 0, 0);

		std::mutex results_mtx;
		std::vector<std::pair<long long, std::string_view>> thread_results;

		auto matrixMultiplicationFunction = [&Result,&thread_results,&results_mtx](const auto* lineA,const auto* columnB, size_t i_index, size_t j_index) {
			Timer timer;
			using vector_type = get_vector_underlying_type_t<std::remove_cvref_t<decltype(*lineA)>>;
			
			auto acc = std::transform(lineA->cbegin(), lineA->cend(), columnB->cbegin(),accumulate_helper<vector_type>{}, [](const auto& first, const auto& second) { return first * second; });
			Result[i_index][j_index] = acc.value(); /// acc tine minte valoarea(copy) la acumulatorul returnat dupa imn liniei si coloanei cu transform

			/// each thread worker how much lasted
			{
				std::scoped_lock<std::mutex> _(results_mtx);
				thread_results.push_back(timer.end_timer());
			}
		};

		Timer global;
		
		ThreadPool threadPool(nr_threads);
		
		/// preload columns , or could have passed B inside lambda capture list as ref and each func creates the column
		std::vector<std::vector<int>> columns(columnsB);
		for (int j = 0; j < columnsB; ++j) {
			columns[j] = B.get_column(j);
		}

		for (int i = 0; i < rowsA; ++i) {

			auto& row = A[i];

			for (int j = 0; j < columnsB; ++j) {
				threadPool.add_task(matrixMultiplicationFunction, &row, &columns[j], i, j); /// each worker processes a different row column multiplication
			}
		}
		
		threadPool.shut_down();
		threadPool.await_termination();

		thread_results.push_back(global.end_timer());
		
		cout << "\n";
		for (const auto& [first, second] : thread_results) {
			std::println("{} {}", first, second);
		}
		
		cout << "\n";
		A.print_matrix();
		cout << "\n";
		B.print_matrix();
		cout << "\n";
		Result.print_matrix();

	}
	
	void merge_sort(auto* threads_results, auto* mutex, auto* _array, auto* _result,size_t st, size_t dr, size_t depth, size_t max_depth) {
		
		if (st >= dr) return;

		size_t mij = (st + dr) / 2;
		auto& array = *_array;
		auto& result = *_result;

		/// Spawns threads at top levels (depth 0, 1, 2, 3) .This creates 2^max_depth threads maximum (16 threads for max_depth=4)
	 
		if (depth < max_depth) {
			std::array<std::future<void>, 2> threads{
				std::async(std::launch::async, [=]() {
					Timer timer;
					merge_sort(threads_results, mutex, _array, _result, st, mij, depth + 1, max_depth);

					std::scoped_lock<std::mutex> _(*mutex);
					threads_results->push_back(timer.end_timer());
				}),
				std::async(std::launch::async, [=]() {
					Timer timer;
					merge_sort(threads_results, mutex, _array, _result, mij + 1, dr, depth + 1, max_depth);

					std::scoped_lock<std::mutex> _(*mutex);
					threads_results->push_back(timer.end_timer());
				})
			};

			threads[0].get();
			threads[1].get();
		}
		else {
			merge_sort(threads_results, mutex, _array, _result, st, mij, depth + 1, max_depth);
			merge_sort(threads_results, mutex, _array, _result, mij + 1, dr, depth + 1, max_depth);
		}

		size_t ind1 = st, ind2 = mij + 1, cnt = st;
		while (ind1 <= mij && ind2 <= dr) {
			if (array[ind1] <= array[ind2])
				result[cnt++] = array[ind1++];
			else
				result[cnt++] = array[ind2++];
		}
		while (ind1 <= mij)
			result[cnt++] = array[ind1++];
		while (ind2 <= dr)
			result[cnt++] = array[ind2++];
		for (size_t i = st; i <= dr; ++i)
			array[i] = result[i];
	}
	void sortArrayParalel(size_t _size,size_t max_depth = 4) {
		
		if (_size <= 0 || max_depth > _size) {
			cout << "Invalid inputs\n";
			return;
		}
		
		std::mutex results_mtx;
		std::vector<std::pair<long long, std::string_view>> thread_results;
		
		Array array(_size, 0, 10000);
		Timer timer;
		auto& arr = array.get_array();
		std::vector<int> aux(_size);

		merge_sort(&thread_results, &results_mtx, &arr, &aux, 0, _size - 1, 0, max_depth);

		thread_results.push_back(timer.end_timer());

		for (const auto& [first, second] : thread_results) {
			std::println("{} {}", first, second);
		}
	}

	void MonteCarloCount(size_t chunk_number, size_t iterations,
		std::reference_wrapper<std::vector<size_t>> results,
		std::reference_wrapper<std::vector<std::pair<long long, std::string_view>>> timings,
		std::reference_wrapper<std::mutex> mtx) {

		Timer timer;

		std::random_device rd;
		std::mt19937 rg(rd());
		std::uniform_real_distribution<double> dist(0.0, 1.0);
		size_t hits{};

		for (size_t i = 0; i < iterations; ++i) {
			auto X = dist(rg);
			auto Y = dist(rg);
			if (X * X + Y * Y < 1) hits += 1;
		}

		results.get()[chunk_number] = hits;

		/// chunk timing
		{
			std::scoped_lock<std::mutex> _(mtx.get());
			timings.get().push_back(timer.end_timer());
		}
	}

	void MonteCarloCountPiEstimation(size_t iterations, size_t nr_threads) {

		if (iterations <= 0 || nr_threads <= 0) {
			cout << "Invalid inputs\n";
			return;
		}

		size_t parallel_split_number = nr_threads * 8; /// create *8 more chunks than threads
		size_t chunk = iterations / parallel_split_number; /// the huge number of iterations(hence better aprox.) is distributed across threads

		std::vector<size_t> results(parallel_split_number);
		std::vector<std::pair<long long, std::string_view>> thread_timings;
		std::mutex timings_mtx;

		Timer total_timer;
		{
			ThreadPool tp(nr_threads);
			for (size_t i{}; i < parallel_split_number; ++i) {
				size_t current_chunk = chunk;
				if (i == (parallel_split_number - 1)) { /// last chunk gets remaining 
					current_chunk = (iterations - i * chunk);
				}
				tp.add_task([i, current_chunk,results_ref = std::ref(results),timings_ref = std::ref(thread_timings),mtx_ref = std::ref(timings_mtx),this](){
						MonteCarloCount(i, current_chunk, results_ref, timings_ref, mtx_ref);
				});
			}
		} /// by this time, all computations are done. Can use result

		thread_timings.push_back(total_timer.end_timer());

		double result = std::accumulate(results.cbegin(), results.cend(), double{}); /// add each individual calc.
		result = 4.0 * result / iterations;
		
		std::println("\nEstimated Pi: {:.10f}", result);
		std::println("Actual Pi:    {:.10f}", std::numbers::pi);
		std::println("Error:        {:.10f}", std::abs(result - std::numbers::pi));

		cout << "\n";
		for (const auto& [first, second] : thread_timings) {
			std::println("{} {}", first, second);
		}
	}
	
	/*void file_words_frequency(int nr_threads) {
		if (!std::filesystem::exists("data.txt")) {
			cout << "file doesnt exist!\n";
			return;
		}
		if (!std::filesystem::is_regular_file("data.txt")) {
			cout << "file not a regular one\n";
			return;
		}

		size_t size = std::filesystem::file_size("data.txt");

		std::ifstream fin("data.txt");

		if (!fin) {
			cout << "coult not open file\n";
			return;
		}

		std::ifstream fin("data.txt");

		if (!fin) {
			cout << "coult not open file\n";
			return;
		}

		fin.seekg(4);
		std::array<char, 1024> arr{ 0 };
		fin.read(arr.data(), 3);

		for (char ch : arr)
			cout << ch;

		cout << "\n";

		char ch;
		fin >> ch;
		while (ch == ' ') {
			fin >> ch;
		}

		fin.read(arr.data(), 4);

		for (char ch : arr)
			cout << ch;

		cout << "\n";
		
		size_t numChunks = nr_threads * 4;
		
		if (size > 1'000'000) numChunks *= 2;
		
		size_t chunkSize = size / numChunks;


	}*/
};

int main()
{
	Tests t;
	t.MonteCarloCountPiEstimation(100'000'000, 32); 
	return 0;
}

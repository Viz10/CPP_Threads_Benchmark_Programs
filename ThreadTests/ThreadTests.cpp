#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <memory>
#include <random>
#include <print>
#include <cassert>

#include "Array.hpp"
#include "Timer.hpp"
#include "ThreadPool.hpp"
#include "MatrixParalel.hpp"

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

class Tests {

public:

	Tests() = default;
	void findMinMax(int nr_threads) {
		
		assert(nr_threads > 0);

		Array array(10000000, 10000, 1000000);
		const auto& arr = array.get_array();
		int size = arr.size();
		int chunk_size = size / nr_threads;

		Timer global;

		std::vector<long long> thread_results(nr_threads + 1);

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
				int end = (i == nr_threads - 1) ? size : (i + 1) * chunk_size; /// last thread gets end of array
				threads.emplace_back(func, i, i * chunk_size, end);
			}
		} /// RAII join jthread , after this all threads have succesfully computed and written time result


		thread_results[nr_threads] = global.end_timer();

		std::ostream_iterator<float> out(cout, "ms\n");
		std::copy(thread_results.begin(), thread_results.end(),out);

	}
};

int main()
{
	Matrix mat(4,1, 0, 100);

	mat.print_matrix();

	return 0;
}

/*
	template <typename T>
	int partition(int st, int dr, std::vector<T>& array, std::unique_ptr<int[]>& times) {
		int i = st, j = dr;
		int mij = (st + dr) / 2;
		int pivot = val[mij];
		std::swap(val[mij], val[dr]);

		while (i < j) {
			while (array[i] <= pivot) {
				i++;
			}
			while (array[j] >= pivot) {
				j++;
			}
			if (i < j) {
				std::swap(a[i], a[j]);
			}
		}
		std::swap(val[i], val[mij]);

		return i;

	}

	template <typename T>
	void QuickSortHelper(int st,int dr,std::vector<T> & array,std::unique_ptr<int[]> & times){
		if (st > dr) return;
		else {
			int pivot = partition(st,dr,array,times);
			QuickSortHelper(st, pivot - 1, array, times);
			QuickSortHelper(pivot + 1,dr, array, times);
		}
	}
	*/

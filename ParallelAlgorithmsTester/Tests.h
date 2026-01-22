#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <thread>
#include <mutex>
#include <random>
#include <numeric>
#include <vector>
#include <algorithm>
#include <future>
#include <numbers>
#include <array>

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

/// iterator like wrapper used for std transform to accumulate result of column line mult.
template<typename T> requires std::is_default_constructible_v<T>
class accumulate_helper {
    T accumalate{};
public:
    T value() { return accumalate; }
    accumulate_helper& operator++() { return *this; }
    accumulate_helper& operator*() { return *this; }
    accumulate_helper& operator=(const T& to_be_added) {
        accumalate += to_be_added;
        return *this;
    }
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
        if (nr_threads == 0) {
            std::cout << "Nr of threads must be > 0" << "\n";
            return;
        }

        Array array(_size, 10000, 1000000);
        const auto& arr = array.get_array();
        size_t size = arr.size();
        size_t chunk_size = size / nr_threads;

        Timer global;
        std::vector<std::pair<long long, std::string_view>> thread_results(nr_threads + 1);
        int min{ INT_MAX }, max{ INT_MIN };
        std::mutex mtx;

        auto func = [&arr, &thread_results, &min, &max, &mtx](int thread_no, int start, int end) {
            Timer timer;
            int local_min = INT_MAX, local_max = INT_MIN;

            for (int i = start; i < end; ++i) {
                if (arr[i] < local_min) local_min = arr[i];
                if (arr[i] > local_max) local_max = arr[i];
            }

            {
                std::scoped_lock<std::mutex> _(mtx);
                if (local_min < min) min = local_min;
                if (local_max > max) max = local_max;
            }

            thread_results[thread_no] = timer.end_timer();
        };

        {
            std::vector<std::jthread> threads;
            for (int i = 0; i < nr_threads; ++i) {
                size_t end = (i == nr_threads - 1) ? size : (i + 1) * chunk_size;
                threads.emplace_back(func, i, i * chunk_size, end);
            }
        } /// auto call join on jthreads

        thread_results[nr_threads] = global.end_timer();


        for (int i = 0; i < nr_threads;++i) {
            cout << "Thread " << i << " finished in " << thread_results[i].first <<" "<< thread_results[i].second << "\n";
        }
        cout<<"\nGlobal execution time was: "<< thread_results[nr_threads].first<<" " << thread_results[nr_threads].second << "\n";
        std::cout << "\nMin: " << min << "\nMax: " << max << "\n";
    }

    void matrixMultiplicationParallel(size_t nr_threads, size_t rowsA, size_t columnsA, size_t rowsB, size_t columnsB) {
        if (columnsA != rowsB) {
            cout << "Size of line elements for A must be equal to size of column elements for B";
            return;
        }

        if (nr_threads==0) {
            std::cout << "Nr of threads must be > 0" << "\n";
            return;
        }

        Matrix<int> A(rowsA, columnsA, 0, 100);
        Matrix<int> B(rowsB, columnsB, 0, 100);
        Matrix<int> Result(rowsA, columnsB, 0, 0);

        std::mutex results_mtx;
        std::vector<std::pair<long long, std::string_view>> thread_results;

        auto matrixMultiplicationFunction = [&Result, &thread_results, &results_mtx](const auto* lineA, const auto* columnB, size_t i_index, size_t j_index) {
            Timer timer;
            using vector_type = get_vector_underlying_type_t<std::remove_cvref_t<decltype(*lineA)>>;

            auto acc = std::transform(lineA->cbegin(), lineA->cend(), columnB->cbegin(), accumulate_helper<vector_type>{},
                                      [](const auto& first, const auto& second) { return first * second; });
            Result[i_index][j_index] = acc.value();

            {
                std::scoped_lock<std::mutex> _(results_mtx);
                thread_results.push_back(timer.end_timer());
            }

        };

        Timer global;
        ThreadPool threadPool(nr_threads);

        std::vector<std::vector<int>> columns(columnsB);
        for (int j = 0; j < columnsB; ++j) {
            columns[j] = B.get_column(j);
        } /// storing columns

        for (int i = 0; i < rowsA; ++i) {
            auto& row = A[i];
            for (int j = 0; j < columnsB; ++j) {
                threadPool.add_task(matrixMultiplicationFunction, &row, &columns[j], i, j);
            }
        }

        threadPool.shut_down(); /// manual force stop
        threadPool.await_termination();

        auto t = global.end_timer();

        cout << "\n\n";
        for (const auto& [first, second] : thread_results) {
            cout << first << " " << second << "\n";
        }

        cout << "\nGlobal execution time was: " << t.first << " " << t.second << "\n";

        cout << "\n\n";
        A.print_matrix();
        cout << "\n\n";
        B.print_matrix();
        cout << "\n\n";
        Result.print_matrix();
    }

    void sortArrayParalel(size_t _size, size_t max_depth = 4) {
        if (_size <= 0 || max_depth > _size) {
            cout << "Invalid inputs\n";
            return;
        }

        std::mutex results_mtx;
        std::vector<std::pair<long long, std::string_view>> thread_results;

        Array array(_size, 0, 100'000);
        Timer timer;
        auto& arr = array.get_array();
        std::vector<int> aux(_size);

        merge_sort(&thread_results, &results_mtx, &arr, &aux, 0, _size - 1, 0, max_depth);

        auto t = timer.end_timer();

        for (const auto& [first, second] : thread_results) {
            cout << first << " " << second << "\n";
        }

        cout << "\nGlobal execution time was: " << t.first << " " << t.second << "\n";
    }

    void MonteCarloCountPiEstimation(size_t iterations, size_t nr_threads) {
        if (iterations == 0 || nr_threads == 0) {
            cout << "Invalid inputs\n";
            return;
        }

        size_t parallel_split_number = nr_threads * 8;
        size_t chunk = iterations / parallel_split_number;

        std::vector<size_t> results(parallel_split_number);
        std::vector<std::pair<long long, std::string_view>> thread_timings;
        std::mutex timings_mtx;

        Timer total_timer;
        {
            ThreadPool tp(nr_threads);
            for (size_t i{}; i < parallel_split_number; ++i) {
                size_t current_chunk = chunk;
                if (i == (parallel_split_number - 1)) {
                    current_chunk = (iterations - i * chunk);
                }
                tp.add_task([i, current_chunk, results_ref = std::ref(results),
                             timings_ref = std::ref(thread_timings),
                             mtx_ref = std::ref(timings_mtx), this]() {
                    MonteCarloCount(i, current_chunk, results_ref, timings_ref, mtx_ref);
                });
            }
        }

        double result = std::accumulate(results.cbegin(), results.cend(), double{});
        result = 4.0 * result / iterations;

        auto t = total_timer.end_timer();
        for (const auto& [first, second] : thread_timings) {
            cout << first << " " << second << "\n";
        }
        cout << "\nGlobal execution time was: " << t.first << " " << t.second << "\n";
        cout << "\n";

        cout << std::fixed << std::setprecision(10);
        cout << "Estimated Pi: " << result << "\n";
        cout << "Actual Pi:    " << std::numbers::pi << "\n";
        cout << "Error:        " << std::abs(result - std::numbers::pi) << std::endl;
    }

    void run_benchmark(size_t max_num_threads, size_t num_tasks, uint64_t work_iterations_per_task) {

        if (max_num_threads == 0 || num_tasks == 0 || work_iterations_per_task == 0) {
            cout << "Invalid Input\n";
            return;
        }

        ///                              nr th   speed                time
        using Wrapper = std::pair<std::pair<size_t, size_t>,std::pair<long long, std::string_view>>;
        std::vector<Wrapper> results;

        Timer timer;
        benchmark_threadpool(1,num_tasks, work_iterations_per_task);
        auto baseline = timer.end_timer(); /// how much a threadpool with 1 thread finished the cpu task

        for (size_t threads = 1; threads <= max_num_threads; threads *= 2) {
            auto t = benchmark_threadpool(threads, num_tasks, work_iterations_per_task);
            size_t speedup = baseline.first / t.first;
            Wrapper item = {{threads,speedup},t};
            results.push_back(std::move(item)); //xrval
        }

        std::cout << "\nBaseline (1 thread): " << baseline.first << baseline.second << "\n\n";

        for (const auto& elem : results) {
            cout << elem.first.first << " Threads:  " <<  elem.second.first << " " << elem.second.second << " with a speedup: " << elem.first.second << "x\n";
        }
    }


private:
    void merge_sort(auto* threads_results, auto* mutex, auto* _array, auto* _result,
                    size_t st, size_t dr, size_t depth, size_t max_depth) {
        if (st >= dr) return;

        size_t mij = (st + dr) / 2;
        auto& array = *_array;
        auto& result = *_result;

        if (depth < max_depth) {
            std::array<std::future<void>, 2> threads{
                std::async(std::launch::async, [=, this]() {
                    Timer timer; /// measure async time
                    merge_sort(threads_results, mutex, _array, _result, st, mij, depth + 1, max_depth);
                    std::scoped_lock<std::mutex> _(*mutex);
                    threads_results->push_back(timer.end_timer());
                }),
                std::async(std::launch::async, [=, this]() {
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
        while (ind1 <= mij) result[cnt++] = array[ind1++];
        while (ind2 <= dr) result[cnt++] = array[ind2++];
        for (size_t i = st; i <= dr; ++i) array[i] = result[i];
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

        {
            std::scoped_lock<std::mutex> _(mtx.get());
            timings.get().push_back(timer.end_timer());
        }
    }

    void cpu_work(std::size_t iterations) {
        volatile std::size_t x = 0; /// prevent CPU from discarded [[unused]] variable (optimization)
        for (std::size_t i = 0; i < iterations; ++i) {
            x += i ^ (x << 1); /// some calculation
        }
    }
    std::pair<long long,std::string_view> benchmark_threadpool(size_t num_threads, size_t num_tasks, uint64_t work_iterations_per_task) {

        std::atomic<size_t> completed{ 0 };
        Timer timer;

        {
            ThreadPool tp(num_threads);
            for (size_t i = 0; i < num_tasks; ++i) {
                tp.add_task([&completed, work_iterations_per_task, this]() {
                    cpu_work(work_iterations_per_task); ++completed;  });
            }
        }

        auto t = timer.end_timer();
        if (completed == num_tasks) {
            return t;
        }
        else {
            cout << "Error\nDid not execute all functions\n";
            exit(-1);
        }
    }
};

#endif // TESTS_H

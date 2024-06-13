#pragma once

#include <oscar/Utils/Algorithms.h>

#include <concepts>
#include <cstddef>
#include <future>
#include <span>
#include <thread>
#include <vector>

namespace osc
{
    // perform a parallelized and "Chunked" ForEach, where each thread receives an
    // independent chunk of data to process
    //
    // this is a poor-man's `std::execution::par_unseq`, because C++17's <execution>
    // isn't fully integrated into MacOS/Ubuntu20
    template<typename T, std::invocable<T&> UnaryFunction>
    void for_each_parallel_unsequenced(
        size_t min_chunk_size,
        std::span<T> values,
        UnaryFunction mutator)
    {
        const size_t chunk_size = max(min_chunk_size, values.size()/std::thread::hardware_concurrency());
        const size_t num_tasks = values.size()/chunk_size;

        if (num_tasks > 1) {
            std::vector<std::future<void>> tasks;
            tasks.reserve(num_tasks);

            for (size_t i = 0; i < num_tasks-1; ++i) {
                const size_t chunk_begin = i * chunk_size;
                const size_t chunk_end = (i+1) * chunk_size;
                tasks.push_back(std::async(std::launch::async, [values, mutator, chunk_begin, chunk_end]()
                {
                    for (size_t j = chunk_begin; j < chunk_end; ++j) {
                        mutator(values[j]);
                    }
                }));
            }

            // last worker must also handle the remainder
            {
                const size_t chunk_begin = (num_tasks-1) * chunk_size;
                const size_t chunk_end = values.size();
                tasks.push_back(std::async(std::launch::async, [values, mutator, chunk_begin, chunk_end]()
                {
                    for (size_t i = chunk_begin; i < chunk_end; ++i) {
                        mutator(values[i]);
                    }
                }));
            }

            for (std::future<void>& task : tasks) {
                task.get();
            }
        }
        else {
            // chunks would be too small if parallelized: just do it sequentially
            for (T& value : values) {
                mutator(value);
            }
        }
    }
}

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
    // isn't fully integrated into MacOS/Linux
    template<typename T, std::invocable<T&> UnaryFunction>
    void ForEachParUnseq(
        size_t minChunkSize,
        std::span<T> vals,
        UnaryFunction f)
    {
        const size_t chunkSize = max(minChunkSize, vals.size()/std::thread::hardware_concurrency());
        const size_t nTasks = vals.size()/chunkSize;

        if (nTasks > 1)
        {
            std::vector<std::future<void>> tasks;
            tasks.reserve(nTasks);

            for (size_t i = 0; i < nTasks-1; ++i)
            {
                const size_t chunkBegin = i * chunkSize;
                const size_t chunkEnd = (i+1) * chunkSize;
                tasks.push_back(std::async(std::launch::async, [vals, f, chunkBegin, chunkEnd]()
                {
                    for (size_t j = chunkBegin; j < chunkEnd; ++j)
                    {
                        f(vals[j]);
                    }
                }));
            }

            // last worker must also handle the remainder
            {
                const size_t chunkBegin = (nTasks-1) * chunkSize;
                const size_t chunkEnd = vals.size();
                tasks.push_back(std::async(std::launch::async, [vals, f, chunkBegin, chunkEnd]()
                {
                    for (size_t i = chunkBegin; i < chunkEnd; ++i)
                    {
                        f(vals[i]);
                    }
                }));
            }

            for (std::future<void>& task : tasks)
            {
                task.get();
            }
        }
        else
        {
            // chunks would be too small if parallelized: just do it sequentially
            for (T& val : vals)
            {
                f(val);
            }
        }
    }
}

#pragma once

#include <nonstd/span.hpp>

#include <cstddef>
#include <future>
#include <thread>
#include <vector>

namespace osc
{
    // perform a parallelized and "Chunked" ForEach, where each thread receives an
    // independent chunk of data to process
    //
    // this is a poor-man's `std::execution::par_unseq`, because C++17's <execution>
    // isn't fully integrated into MacOS/Linux
    template<typename T, typename UnaryFunction>
    void ForEachParUnseq(size_t minChunkSize, nonstd::span<T> vals, UnaryFunction f)
    {
        size_t const chunkSize = std::max(minChunkSize, vals.size()/std::thread::hardware_concurrency());
        size_t const remainder = vals.size() % chunkSize;
        size_t const nTasks = vals.size()/chunkSize;

        if (nTasks > 1)
        {
            std::vector<std::future<void>> tasks;
            tasks.reserve(nTasks);

            for (size_t i = 0; i < nTasks-1; ++i)
            {
                size_t const chunkBegin = i * chunkSize;
                size_t const chunkEnd = (i+1) * chunkSize;
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
                size_t const chunkBegin = (nTasks-1) * chunkSize;
                size_t const chunkEnd = vals.size();
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

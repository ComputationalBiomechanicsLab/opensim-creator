#include "Perf.hpp"

#include "oscar/Utils/Algorithms.hpp"
#include "oscar/Utils/SynchronizedValue.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
    int64_t GenerateID(
        char const* label,
        char const* filename,
        unsigned int line)
    {
        return static_cast<int64_t>(osc::HashOf(std::string{label}, std::string{filename}, line));
    }

    osc::SynchronizedValue<std::unordered_map<int64_t, osc::PerfMeasurement>>& GetMeasurementStorage()
    {
        static osc::SynchronizedValue<std::unordered_map<int64_t, osc::PerfMeasurement>> s_Measurements;
        return s_Measurements;
    }
}


// public API

int64_t osc::AllocateMeasurementID(char const* label, char const* filename, unsigned int line)
{
    int64_t id = GenerateID(label, filename, line);

    auto guard = GetMeasurementStorage().lock();
    guard->emplace(std::piecewise_construct, std::tie(id), std::tie(id, label, filename, line));
    return id;
}

void osc::SubmitMeasurement(int64_t id, PerfClock::time_point start, PerfClock::time_point end) noexcept
{
    auto guard = GetMeasurementStorage().lock();
    auto it = guard->find(id);

    if (it != guard->end())
    {
        it->second.submit(start, end);
    }
}

void osc::ClearPerfMeasurements()
{
    auto guard = GetMeasurementStorage().lock();

    for (auto& [id, data] : *guard)
    {
        data.clear();
    }
}

size_t osc::GetAllMeasurements(std::vector<PerfMeasurement>& appendOut)
{
    auto guard = GetMeasurementStorage().lock();
    size_t i = 0;
    for (auto const& [id, measurement] : *guard)
    {
        appendOut.push_back(measurement);
        ++i;
    }
    return i;
}





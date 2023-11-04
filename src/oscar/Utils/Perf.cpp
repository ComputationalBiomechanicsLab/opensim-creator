#include "Perf.hpp"

#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
    int64_t GenerateID(
        std::string_view label,
        std::string_view filename,
        unsigned int line)
    {
        return static_cast<int64_t>(osc::HashOf(label, filename, line));
    }

    osc::SynchronizedValue<std::unordered_map<int64_t, osc::PerfMeasurement>>& GetMeasurementStorage()
    {
        static osc::SynchronizedValue<std::unordered_map<int64_t, osc::PerfMeasurement>> s_Measurements;
        return s_Measurements;
    }
}


// public API

int64_t osc::detail::AllocateMeasurementID(
    std::string_view label,
    std::source_location location)
{
    int64_t id = GenerateID(label, location.file_name(), location.line());
    auto metadata = std::make_shared<PerfMeasurementMetadata>(id, label, location.file_name(), location.line());

    auto guard = GetMeasurementStorage().lock();
    guard->emplace(std::piecewise_construct, std::tie(id), std::tie(metadata));
    return id;
}

void osc::detail::SubmitMeasurement(int64_t id, PerfClock::time_point start, PerfClock::time_point end) noexcept
{
    auto guard = GetMeasurementStorage().lock();
    auto it = guard->find(id);

    if (it != guard->end())
    {
        it->second.submit(start, end);
    }
}

void osc::ClearAllPerfMeasurements()
{
    auto guard = GetMeasurementStorage().lock();

    for (auto& [id, data] : *guard)
    {
        data.clear();
    }
}

std::vector<osc::PerfMeasurement> osc::GetAllPerfMeasurements()
{
    auto guard = GetMeasurementStorage().lock();

    std::vector<PerfMeasurement> rv;
    rv.reserve(guard->size());
    for (auto const& [id, measurement] : *guard)
    {
        rv.push_back(measurement);
    }
    return rv;
}

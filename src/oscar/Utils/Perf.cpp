#include "Perf.h"

#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/SynchronizedValue.h>

#include <ankerl/unordered_dense.h>

#include <unordered_map>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    size_t generate_perf_measurement_id(
        std::string_view label,
        std::string_view filename,
        unsigned int line)
    {
        return hash_of(label, filename, line);
    }

    SynchronizedValue<ankerl::unordered_dense::map<size_t, PerfMeasurement>>& get_global_perf_measurement_storage()
    {
        static SynchronizedValue<ankerl::unordered_dense::map<size_t, PerfMeasurement>> s_measurement_storage;
        return s_measurement_storage;
    }
}

size_t osc::detail::allocate_perf_mesurement_id(std::string_view label, std::string_view filename, unsigned int line)
{
    size_t id = generate_perf_measurement_id(label, filename, line);
    auto metadata = std::make_shared<PerfMeasurementMetadata>(id, label, filename, line);

    auto guard = get_global_perf_measurement_storage().lock();
    guard->emplace(std::piecewise_construct, std::tie(id), std::tie(metadata));
    return id;
}

void osc::detail::submit_perf_measurement(size_t id, PerfClock::time_point start, PerfClock::time_point end)
{
    auto guard = get_global_perf_measurement_storage().lock();

    if (const auto it = guard->find(id); it != guard->end()) {
        it->second.submit(start, end);
    }
}

void osc::clear_all_perf_measurements()
{
    auto guard = get_global_perf_measurement_storage().lock();

    for (auto& [id, data] : *guard) {
        data.clear();
    }
}

std::vector<PerfMeasurement> osc::get_all_perf_measurements()
{
    auto guard = get_global_perf_measurement_storage().lock();

    std::vector<PerfMeasurement> rv;
    rv.reserve(guard->size());
    for (const auto& [id, measurement] : *guard) {
        rv.push_back(measurement);
    }
    return rv;
}

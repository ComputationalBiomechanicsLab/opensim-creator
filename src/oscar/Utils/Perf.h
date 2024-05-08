#pragma once

#include <oscar/Utils/FilenameExtractor.h>
#include <oscar/Utils/PerfClock.h>
#include <oscar/Utils/PerfMeasurement.h>

#include <cstdint>
#include <string_view>
#include <vector>

namespace osc
{
    void clear_all_perf_measurements();
    std::vector<PerfMeasurement> get_all_perf_measurements();

    // internal details needed for `OSC_PERF` to work
    namespace detail
    {
        size_t allocate_perf_mesurement_id(
            std::string_view label,
            std::string_view filename,
            unsigned int line
        );

        void submit_perf_measurement(
            size_t id,
            PerfClock::time_point start,
            PerfClock::time_point end
        );

        class PerfTimer final {
        public:
            explicit PerfTimer(size_t id) : id_{id} {}
            PerfTimer(PerfTimer const&) = delete;
            PerfTimer(PerfTimer&&) noexcept = delete;
            PerfTimer& operator=(const PerfTimer&) = delete;
            PerfTimer& operator=(PerfTimer&&) noexcept = delete;
            ~PerfTimer() noexcept
            {
                submit_perf_measurement(id_, start_time_, PerfClock::now());
            }
        private:
            size_t id_;
            PerfClock::time_point start_time_ = PerfClock::now();
        };
    }
}

#define OSC_PERF_TOKENPASTE(x, y) x##y
#define OSC_PERF_TOKENPASTE2(x, y) OSC_PERF_TOKENPASTE(x, y)
#define OSC_PERF(label) \
    static const size_t OSC_PERF_TOKENPASTE2(s_TimerID, __LINE__) = osc::detail::allocate_perf_mesurement_id(label, osc::extract_filename(__FILE__), __LINE__); \
    const osc::detail::PerfTimer OSC_PERF_TOKENPASTE2(timer, __LINE__) (OSC_PERF_TOKENPASTE2(s_TimerID, __LINE__));

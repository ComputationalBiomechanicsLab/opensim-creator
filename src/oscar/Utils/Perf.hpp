#pragma once

#include <oscar/Utils/FilenameExtractor.hpp>
#include <oscar/Utils/PerfClock.hpp>
#include <oscar/Utils/PerfMeasurement.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace osc
{
    void ClearAllPerfMeasurements();
    std::vector<PerfMeasurement> GetAllPerfMeasurements();

    // internal details needed for `OSC_PERF` to work
    namespace detail
    {
        int64_t AllocateMeasurementID(
            std::string_view label,
            std::string_view filename,
            unsigned int line
        );

        void SubmitMeasurement(
            int64_t id,
            PerfClock::time_point start,
            PerfClock::time_point end
        ) noexcept;

        class PerfTimer final {
        public:
            explicit PerfTimer(int64_t id) noexcept :
                m_ID{id}
            {
            }
            PerfTimer(PerfTimer const&) = delete;
            PerfTimer(PerfTimer&&) noexcept = delete;
            PerfTimer& operator=(PerfTimer const&) = delete;
            PerfTimer& operator=(PerfTimer&&) noexcept = delete;
            ~PerfTimer() noexcept
            {
                SubmitMeasurement(m_ID, m_Start, PerfClock::now());
            }
        private:
            int64_t m_ID;
            PerfClock::time_point m_Start = PerfClock::now();
        };
    }
}

#define OSC_PERF_TOKENPASTE(x, y) x##y
#define OSC_PERF_TOKENPASTE2(x, y) OSC_PERF_TOKENPASTE(x, y)
#define OSC_PERF(label) \
    static int64_t const OSC_PERF_TOKENPASTE2(s_TimerID, __LINE__) = osc::detail::AllocateMeasurementID(label, osc::ExtractFilename(__FILE__), __LINE__); \
    osc::detail::PerfTimer const OSC_PERF_TOKENPASTE2(timer, __LINE__) (OSC_PERF_TOKENPASTE2(s_TimerID, __LINE__));

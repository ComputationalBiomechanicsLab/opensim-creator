#pragma once

#include "src/Assertions.hpp"
#include "src/Utils/ScopeGuard.hpp"

#include <chrono>
#include <cstdint>

namespace osc
{
    using PerfClock = std::chrono::high_resolution_clock;

    int64_t AllocateMeasurementID(char const* label, char const* filename, unsigned int line);
    void SubmitMeasurement(int64_t id, PerfClock::time_point start, PerfClock::time_point end);
    void PrintMeasurementsToLog();

    class PerfTimer {
    public:
        explicit PerfTimer(int64_t id) : m_ID{id} {}
        PerfTimer(PerfTimer const&) = delete;
        PerfTimer(PerfTimer&&) noexcept = delete;
        ~PerfTimer() noexcept
        {
            SubmitMeasurement(m_ID, m_Start, PerfClock::now());
        }
        PerfTimer& operator=(PerfTimer const&) = delete;
        PerfTimer& operator=(PerfTimer&&) noexcept = delete;
    private:
        int64_t m_ID;
        PerfClock::time_point m_Start = PerfClock::now();
    };

#define OSC_PERF(label) \
    static int64_t const OSC_TOKENPASTE2(g_TimerID, __LINE__) = AllocateMeasurementID(label, OSC_FILENAME, __LINE__); \
    PerfTimer OSC_TOKENPASTE2(timer, __LINE__) (OSC_TOKENPASTE2(g_TimerID, __LINE__));
}

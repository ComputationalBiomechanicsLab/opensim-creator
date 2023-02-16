#pragma once

#include "src/Utils/Macros.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osc
{
    using PerfClock = std::chrono::high_resolution_clock;

    class PerfMeasurement final {
    public:
        PerfMeasurement(int64_t id,
                        char const* label,
                        char const* filename,
                        unsigned int line) :
            m_ID{id},
            m_Label{label},
            m_Filename{filename},
            m_Line{line}
        {
        }

        int64_t getID() const
        {
            return m_ID;
        }

        std::string const& getLabel() const
        {
            return m_Label;
        }

        std::string const& getFilename() const
        {
            return m_Filename;
        }

        unsigned int getLine() const
        {
            return m_Line;
        }

        int64_t getCallCount() const
        {
            return m_CallCount;
        }

        osc::PerfClock::duration getLastDuration() const
        {
            return m_LastDuration;
        }

        osc::PerfClock::duration getAvgDuration() const
        {
            return m_CallCount > 0 ? m_TotalDuration/m_CallCount : osc::PerfClock::duration{0};
        }

        osc::PerfClock::duration getTotalDuration() const
        {
            return m_TotalDuration;
        }

        void submit(osc::PerfClock::time_point start, osc::PerfClock::time_point end)
        {
            m_LastDuration = end - start;
            m_TotalDuration += m_LastDuration;
            m_CallCount++;
        }

        void clear()
        {
            m_CallCount = 0;
            m_TotalDuration = osc::PerfClock::duration{0};
            m_LastDuration = osc::PerfClock::duration{0};
        }

    private:
        int64_t m_ID;
        std::string m_Label;
        std::string m_Filename;
        unsigned int m_Line = 0;

        int64_t m_CallCount = 0;
        osc::PerfClock::duration m_TotalDuration{0};
        osc::PerfClock::duration m_LastDuration{0};
    };

    int64_t AllocateMeasurementID(char const* label, char const* filename, unsigned int line);
    void SubmitMeasurement(int64_t id, PerfClock::time_point start, PerfClock::time_point end) noexcept;
    void ClearPerfMeasurements();
    size_t GetAllMeasurements(std::vector<PerfMeasurement>& appendOut);

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

#define OSC_PERF(label) \
    static int64_t const OSC_TOKENPASTE2(s_TimerID, __LINE__) = osc::AllocateMeasurementID(label, OSC_FILENAME, __LINE__); \
    osc::PerfTimer OSC_TOKENPASTE2(timer, __LINE__) (OSC_TOKENPASTE2(s_TimerID, __LINE__));
}

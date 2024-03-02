#pragma once

#include <oscar/Utils/PerfClock.h>
#include <oscar/Utils/PerfMeasurementMetadata.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace osc
{
    class PerfMeasurement final {
    public:
        PerfMeasurement(
            std::shared_ptr<PerfMeasurementMetadata const> const& metadata_) :
            m_Metadata{metadata_}
        {
        }

        size_t getID() const
        {
            return m_Metadata->getID();
        }

        std::string const& getLabel() const
        {
            return m_Metadata->getLabel();
        }

        std::string const& getFilename() const
        {
            return m_Metadata->getFilename();
        }

        unsigned int getLine() const
        {
            return m_Metadata->getLine();
        }

        size_t getCallCount() const
        {
            return m_CallCount;
        }

        PerfClock::duration getLastDuration() const
        {
            return m_LastDuration;
        }

        PerfClock::duration getAvgDuration() const
        {
            return m_CallCount > 0 ? m_TotalDuration/static_cast<ptrdiff_t>(m_CallCount) : PerfClock::duration{0};
        }

        PerfClock::duration getTotalDuration() const
        {
            return m_TotalDuration;
        }

        void submit(PerfClock::time_point start, PerfClock::time_point end)
        {
            m_LastDuration = end - start;
            m_TotalDuration += m_LastDuration;
            m_CallCount++;
        }

        void clear()
        {
            m_CallCount = 0;
            m_TotalDuration = PerfClock::duration{0};
            m_LastDuration = PerfClock::duration{0};
        }

    private:
        std::shared_ptr<PerfMeasurementMetadata const> m_Metadata;
        size_t m_CallCount = 0;
        PerfClock::duration m_TotalDuration{0};
        PerfClock::duration m_LastDuration{0};
    };
}

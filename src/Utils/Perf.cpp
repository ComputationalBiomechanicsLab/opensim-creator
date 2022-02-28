#include "Perf.hpp"

#include "src/Utils/Algorithms.hpp"
#include "src/Utils/SynchronizedValue.hpp"
#include "src/Log.hpp"

#include <unordered_map>
#include <mutex>
#include <iostream>
#include <string>
#include <sstream>

namespace
{
    int64_t GenerateID(char const* label, char const* filename, unsigned int line)
    {
        return static_cast<int64_t>(osc::HashOf(std::string{label}, std::string{filename}, line));
    }

    class MeasurementData final {
    public:
        MeasurementData(char const* label, char const* filename, unsigned int line) :
            m_Label{label},
            m_Filename{filename},
            m_Line{line}
        {
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
        std::string m_Label;
        std::string m_Filename;
        unsigned int m_Line = 0;

        int64_t m_CallCount = 0;
        osc::PerfClock::duration m_TotalDuration{0};
        osc::PerfClock::duration m_LastDuration{0};
    };

    std::ostream& operator<<(std::ostream& o, MeasurementData const& md)
    {
        return o << md.getLabel() << " (" << md.getFilename() << ':' << md.getLine() << ") " << md.getCallCount() << " calls, avg. duration = " << std::chrono::duration_cast<std::chrono::microseconds>(md.getAvgDuration()).count() << " us, last = " << std::chrono::duration_cast<std::chrono::microseconds>(md.getLastDuration()).count() << " us";
    }

    osc::SynchronizedValue<std::unordered_map<int64_t, MeasurementData>>& GetMeasurementStorage()
    {
        static osc::SynchronizedValue<std::unordered_map<int64_t, MeasurementData>> g_Measurements;
        return g_Measurements;
    }
}

int64_t osc::AllocateMeasurementID(char const* label, char const* filename, unsigned int line)
{
    int64_t id = GenerateID(label, filename, line);

    auto guard = GetMeasurementStorage().lock();
    guard->emplace(std::piecewise_construct, std::tie(id), std::tie(label, filename, line));
    return id;
}

void osc::SubmitMeasurement(int64_t id, PerfClock::time_point start, PerfClock::time_point end)
{
    auto guard = GetMeasurementStorage().lock();
    auto it = guard->find(id);

    if (it != guard->end())
    {
        it->second.submit(start, end);
    }
}

void osc::PrintMeasurementsToLog()
{
    auto guard = GetMeasurementStorage().lock();

    for (auto const& [id, data] : *guard)
    {
        std::stringstream ss;
        ss << data;
        log::trace("%s", std::move(ss).str().c_str());
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

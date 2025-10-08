#pragma once

#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>

#include <liboscar/Variant/Variant.h>

#include <functional>
#include <utility>

namespace osc
{
    // encapsulates a function that can extract a single output value from a `SimulationReport`
    //
    // be careful about lifetimes: these value extractors are usually "tied" to a component that
    // they're extracting from, so it's handy to ensure that the callback function has proper
    // lifetime management (e.g. refcounted pointers or similar)
    class OutputValueExtractor final {
    public:
        static OutputValueExtractor constant(Variant value)
        {
            return OutputValueExtractor{std::move(value)};
        }

        explicit OutputValueExtractor(std::function<Variant(const SimulationReport&)> callback_) :
            m_Callback{std::move(callback_)}
        {}

        Variant operator()(const SimulationReport& report) const { return m_Callback(report); }
    private:
        explicit OutputValueExtractor(Variant value) :
            m_Callback{[v = std::move(value)](const SimulationReport&) { return v; }}
        {}

        std::function<Variant(const SimulationReport&)> m_Callback;
    };
}

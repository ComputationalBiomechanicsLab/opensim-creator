#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractorDataType.h>

#include <oscar/Variant.h>

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
        explicit OutputValueExtractor(std::function<Variant(SimulationReport const&)> callback_) :
            m_Callback{std::move(callback_)}
        {}

        Variant operator()(SimulationReport const& report) const { return m_Callback(report); }
    private:
        std::function<Variant(SimulationReport const&)> m_Callback;
    };
}

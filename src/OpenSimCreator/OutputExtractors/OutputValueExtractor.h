#pragma once

#include <oscar/Variant.h>

#include <functional>
#include <utility>

namespace osc { class ISimulationState; }

namespace osc
{
    // encapsulates a function that can extract a single output value from a `ISimulationState`
    //
    // be careful about lifetimes: these value extractors are usually "tied" to a component that
    // they're extracting from, so it's handy to ensure that the callback function has proper
    // lifetime management (e.g. refcounted pointers to the `OpenSim::Component` or similar)
    class OutputValueExtractor final {
    public:
        explicit OutputValueExtractor(std::function<Variant(ISimulationState const&)> callback_) :
            m_Callback{std::move(callback_)}
        {}

        Variant operator()(ISimulationState const& state) const { return m_Callback(state); }
    private:
        std::function<Variant(ISimulationState const&)> m_Callback;
    };
}

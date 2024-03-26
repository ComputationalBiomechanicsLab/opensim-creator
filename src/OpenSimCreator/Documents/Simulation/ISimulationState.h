#pragma once

#include <oscar/Utils/UID.h>

#include <optional>

namespace SimTK { class State; }

namespace osc
{
    // an interface to a (presumed-to-be) realized `SimTK::State`, plus metadata
    class ISimulationState {
    protected:
        ISimulationState() = default;
        ISimulationState(ISimulationState const&) = default;
        ISimulationState(ISimulationState&&) noexcept = default;
        ISimulationState& operator=(ISimulationState const&) = default;
        ISimulationState& operator=(ISimulationState&&) noexcept = default;
    public:
        virtual ~ISimulationState() noexcept = default;

        SimTK::State const& state() const { return implGetState(); }
        std::optional<float> findAuxiliaryValue(UID id) const { return implFindAuxiliaryValue(id); }
    private:
        virtual SimTK::State const& implGetState() const = 0;
        virtual std::optional<float> implFindAuxiliaryValue(UID) const { return std::nullopt; }
    };
}

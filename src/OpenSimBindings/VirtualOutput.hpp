#pragma once

#include "src/Utils/UID.hpp"

#include <optional>
#include <string>

namespace OpenSim
{
    class Component;
}

namespace osc
{
    class SimulationReport;
}

namespace osc
{
    // tag that indicates what type the output has (handy for UI grouping etc)
    enum class OutputSource {
        Integrator = 0,
        UserEnacted,
        System,
        Simulator,
    };

    // type-erased virtual interface to some underlying (concrete) output implementation
    class VirtualOutput {
    public:
        virtual ~VirtualOutput() noexcept = default;
        virtual UID getID() const = 0;
        virtual OutputSource getOutputSource() const = 0;
        virtual std::string const& getName() const = 0;
        virtual std::string const& getDescription() const = 0;
        virtual bool producesNumericValues() const = 0;
        virtual std::optional<float> getNumericValue(OpenSim::Component const&, SimulationReport const&) const = 0;
        virtual std::optional<std::string> getStringValue(OpenSim::Component const&, SimulationReport const&) const = 0;
    };
}

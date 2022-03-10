#pragma once

#include "src/Utils/UID.hpp"

#include <nonstd/span.hpp>

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
    // indicates where the output comes from - handy for UI grouping
    enum class OutputSource {
        Integrator = 0,
        UserEnacted,
        System,
        Simulator,
        TOTAL,
    };

    // indicates the datatype that the output emits - callers should check this
    enum class OutputType {
        Float = 0,
        String,
        TOTAL,
    };

    // type-erased virtual interface to some underlying (concrete) output implementation
    class VirtualOutput {
    public:
        virtual ~VirtualOutput() noexcept = default;

        virtual UID getID() const = 0;
        virtual OutputSource getOutputSource() const = 0;
        virtual std::string const& getName() const = 0;
        virtual std::string const& getDescription() const = 0;

        virtual OutputType getOutputType() const = 0;
        virtual float getValueFloat(OpenSim::Component const&, SimulationReport const&) const = 0;
        virtual void getValuesFloat(OpenSim::Component const&,
                                    nonstd::span<SimulationReport const>,
                                    nonstd::span<float> overwriteOut) const = 0;
        virtual std::string getValueString(OpenSim::Component const&, SimulationReport const&) const = 0;
    };
}

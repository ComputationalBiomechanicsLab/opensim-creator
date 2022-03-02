#pragma once

#include "src/OpenSimBindings/VirtualOutput.hpp"
#include "src/Utils/ClonePtr.hpp"
#include "src/Utils/UID.hpp"

#include <optional>
#include <string_view>
#include <string>

namespace OpenSim
{
    class AbstractOutput;
    class ComponentPath;
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    class SimulationReport;
}

namespace osc
{
    class ModelOutput final : public VirtualOutput {
    public:
        using ExtractorFn = float (*)(OpenSim::AbstractOutput const&, SimTK::State const&);

        ModelOutput(OpenSim::ComponentPath const& absPath,
                    std::string_view outputName);
        ModelOutput(OpenSim::ComponentPath const& absPath,
                    std::string_view outputName,
                    ExtractorFn subfieldExtractor);

        UID getID() const override;
        OutputType getOutputType() const override;
        std::string const& getName() const override;
        std::string const& getDescription() const override;
        bool producesNumericValues() const override;
        std::optional<float> getNumericValue(OpenSim::Model const&, SimulationReport const&) const override;
        std::optional<std::string> getStringValue(OpenSim::Model const&, SimulationReport const&) const override;

        class Impl;
    private:
        ClonePtr<Impl> m_Impl;
    };
}

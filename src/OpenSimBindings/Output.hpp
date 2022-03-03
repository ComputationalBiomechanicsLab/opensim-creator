#pragma once

#include "src/OpenSimBindings/VirtualOutput.hpp"
#include "src/Utils/UID.hpp"

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <utility>


namespace OpenSim
{
    class Component;
}

namespace osc
{
    class SimulationReport;
}

namespace SimTK
{
    class State;
    class Integrator;
}


namespace osc
{
    // concrete reference-counted output value-type that can be compared, hashed,
    // etc. for easy usage by other parts of osc (e.g. aggregators, plotters)
    class Output {
    public:
        template<class SpecificOutput>
        Output(SpecificOutput&& output) :
            m_Output{std::make_shared<SpecificOutput>(std::forward<SpecificOutput>(output))}
        {
        }

        UID getID() const;
        OutputType getOutputType() const;
        std::string const& getName() const;
        std::string const& getDescription() const;
        bool producesNumericValues() const;
        std::optional<float> getNumericValue(OpenSim::Component const&, SimulationReport const&) const;
        std::optional<std::string> getStringValue(OpenSim::Component const&, SimulationReport const&) const;
        VirtualOutput const& getInner() const;

    private:
        friend bool operator==(Output const&, Output const&);
        friend bool operator!=(Output const&, Output const&);
        friend bool operator<(Output const&, Output const&);
        friend bool operator<=(Output const&, Output const&);
        friend bool operator>(Output const&, Output const&);
        friend bool operator>=(Output const&, Output const&);
        friend std::ostream& operator<<(std::ostream&, Output const&);
        friend std::string to_string(Output const&);
        friend struct std::hash<Output>;

        std::shared_ptr<VirtualOutput> m_Output;
    };

    bool operator==(Output const& a, Output const& b);
    bool operator!=(Output const& a, Output const& b);
    bool operator<(Output const& a, Output const& b);
    bool operator<=(Output const& a, Output const& b);
    bool operator>(Output const& a, Output const& b);
    bool operator>=(Output const& a, Output const& b);
    std::ostream& operator<<(std::ostream&, Output const&);
    std::string to_string(Output const&);
}

namespace std
{
    template<>
    struct hash<osc::Output> {
        std::size_t operator()(osc::Output const&) const;
    };
}

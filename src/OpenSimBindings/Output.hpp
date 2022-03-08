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

        UID getID() const { return m_Output->getID(); }
        OutputSource getOutputSource() const { return m_Output->getOutputSource(); }
        std::string const& getName() const { return m_Output->getName(); }
        std::string const& getDescription() const { return m_Output->getDescription(); }
        bool producesNumericValues() const { return m_Output->producesNumericValues(); }
        std::optional<float> getNumericValue(OpenSim::Component const& c, SimulationReport const& sr) const { return m_Output->getNumericValue(c, sr); }
        std::optional<std::string> getStringValue(OpenSim::Component const& c, SimulationReport const& sr) const { return m_Output->getStringValue(c, sr); }

        VirtualOutput const& getInner() const { return *m_Output; }
        operator VirtualOutput const& () const { return *m_Output; }
        operator VirtualOutput& () { return *m_Output; }

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

    inline bool operator==(Output const& a, Output const& b) { return a.m_Output == b.m_Output; }
    inline bool operator!=(Output const& a, Output const& b) { return a.m_Output != b.m_Output; }
    inline bool operator<(Output const& a, Output const& b) { return a.m_Output < b.m_Output; }
    inline bool operator<=(Output const& a, Output const& b) { return a.m_Output <= b.m_Output; }
    inline bool operator>(Output const& a, Output const& b) { return a.m_Output > b.m_Output; }
    inline bool operator>=(Output const& a, Output const& b) { return a.m_Output >= b.m_Output; }
    std::ostream& operator<<(std::ostream&, Output const&);
    std::string to_string(Output const&);
}

namespace std
{
    template<>
    struct hash<osc::Output> {
        std::size_t operator()(osc::Output const& o) const
        {
            return std::hash<std::shared_ptr<osc::VirtualOutput>>{}(o.m_Output);
        }
    };
}

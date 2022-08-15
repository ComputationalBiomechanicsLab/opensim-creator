#pragma once

#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"

#include <nonstd/span.hpp>

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <utility>


namespace OpenSim { class Component; }
namespace osc { class SimulationReport; }

namespace osc
{
    // concrete reference-counted value-type wrapper for an `osc::VirtualOutputExtractor`.
    //
    // This is a value-type that can be compared, hashed, etc. for easier usage
    // by other parts of osc (e.g. aggregators, plotters)
    class OutputExtractor {
    public:
        template<class SpecificOutput>
        explicit OutputExtractor(SpecificOutput&& output) :
            m_Output{std::make_shared<SpecificOutput>(std::forward<SpecificOutput>(output))}
        {
        }
        std::string const& getName() const
        {
            return m_Output->getName();
        }
        std::string const& getDescription() const
        {
            return m_Output->getDescription();
        }
        OutputType getOutputType() const
        {
            return m_Output->getOutputType();
        }
        float getValueFloat(OpenSim::Component const& c, SimulationReport const& r) const
        {
            return m_Output->getValueFloat(c, r);
        }
        void getValuesFloat(OpenSim::Component const& c,
                            nonstd::span<SimulationReport const> reports,
                            nonstd::span<float> overwriteOut)
        {
            m_Output->getValuesFloat(c, reports, overwriteOut);
        }
        std::string getValueString(OpenSim::Component const& c, SimulationReport const& r) const
        {
            return m_Output->getValueString(c, r);
        }

        VirtualOutputExtractor const& getInner() const { return *m_Output; }
        operator VirtualOutputExtractor const& () const { return *m_Output; }
        operator VirtualOutputExtractor& () { return *m_Output; }

    private:
        friend bool operator==(OutputExtractor const&, OutputExtractor const&);
        friend bool operator!=(OutputExtractor const&, OutputExtractor const&);
        friend std::string to_string(OutputExtractor const&);
        friend struct std::hash<OutputExtractor>;

        std::shared_ptr<VirtualOutputExtractor> m_Output;
    };

    inline bool operator==(OutputExtractor const& a, OutputExtractor const& b) { return *a.m_Output == *b.m_Output; }
    inline bool operator!=(OutputExtractor const& a, OutputExtractor const& b) { return *a.m_Output != *b.m_Output; }
    std::ostream& operator<<(std::ostream&, OutputExtractor const&);
    std::string to_string(OutputExtractor const&);
}

namespace std
{
    template<>
    struct hash<osc::OutputExtractor> {
        std::size_t operator()(osc::OutputExtractor const& o) const
        {
            return o.m_Output->getHash();
        }
    };
}

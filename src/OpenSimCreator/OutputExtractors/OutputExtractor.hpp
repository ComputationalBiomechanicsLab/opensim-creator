#pragma once

#include <OpenSimCreator/OutputExtractors/IOutputExtractor.hpp>

#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <span>
#include <string>
#include <utility>


namespace OpenSim { class Component; }
namespace osc { class SimulationReport; }

namespace osc
{
    // concrete reference-counted value-type wrapper for an `IOutputExtractor`.
    //
    // This is a value-type that can be compared, hashed, etc. for easier usage
    // by other parts of osc (e.g. aggregators, plotters)
    class OutputExtractor final : public IOutputExtractor {
    public:
        template<typename SpecificOutput>
        explicit OutputExtractor(SpecificOutput&& output) :
            m_Output{std::make_shared<SpecificOutput>(std::forward<SpecificOutput>(output))}
        {
        }

        CStringView getName() const final
        {
            return m_Output->getName();
        }

        CStringView getDescription() const final
        {
            return m_Output->getDescription();
        }

        OutputType getOutputType() const final
        {
            return m_Output->getOutputType();
        }

        float getValueFloat(OpenSim::Component const& c, SimulationReport const& r) const final
        {
            return m_Output->getValueFloat(c, r);
        }

        void getValuesFloat(
            OpenSim::Component const& c,
            std::span<SimulationReport const> reports,
            std::span<float> overwriteOut) const final
        {
            m_Output->getValuesFloat(c, reports, overwriteOut);
        }

        std::string getValueString(OpenSim::Component const& c, SimulationReport const& r) const final
        {
            return m_Output->getValueString(c, r);
        }

        size_t getHash() const final
        {
            return m_Output->getHash();
        }

        bool equals(IOutputExtractor const& other) const final
        {
            return m_Output->equals(other);
        }

        IOutputExtractor const& getInner() const
        {
            return *m_Output;
        }

        friend bool operator==(OutputExtractor const& lhs, OutputExtractor const& rhs)
        {
            return *lhs.m_Output == *rhs.m_Output;
        }
    private:
        friend std::string to_string(OutputExtractor const&);
        friend struct std::hash<OutputExtractor>;

        std::shared_ptr<IOutputExtractor> m_Output;
    };

    std::ostream& operator<<(std::ostream&, OutputExtractor const&);
    std::string to_string(OutputExtractor const&);
}

template<>
struct std::hash<osc::OutputExtractor> final {
    size_t operator()(osc::OutputExtractor const& o) const
    {
        return o.m_Output->getHash();
    }
};

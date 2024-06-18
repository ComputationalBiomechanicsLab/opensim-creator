#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <oscar/Utils/CStringView.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace OpenSim { class Component; }

namespace osc
{
    // concrete reference-counted value-type wrapper for an `IOutputExtractor`.
    //
    // This is a value-type that can be compared, hashed, etc. for easier usage
    // by other parts of osc (e.g. aggregators, plotters)
    class OutputExtractor final {
    public:
        template<typename ConcreteIOutputExtractor>
        explicit OutputExtractor(ConcreteIOutputExtractor&& output) :
            m_Output{std::make_shared<ConcreteIOutputExtractor>(std::forward<ConcreteIOutputExtractor>(output))}
        {}

        CStringView getName() const { return m_Output->getName(); }
        CStringView getDescription() const { return m_Output->getDescription(); }
        OutputExtractorDataType getOutputType() const { return m_Output->getOutputType(); }

        OutputValueExtractor getOutputValueExtractor(const OpenSim::Component& component) const
        {
            return m_Output->getOutputValueExtractor(component);
        }

        float getValueFloat(const OpenSim::Component& component, const SimulationReport& report) const
        {
            return m_Output->getValueFloat(component, report);
        }

        void getValuesFloat(
            const OpenSim::Component& component,
            std::span<SimulationReport const> reports,
            std::function<void(float)> const& consumer) const
        {
            m_Output->getValuesFloat(component, reports, consumer);
        }

        std::vector<float> slurpValuesFloat(
            const OpenSim::Component& component,
            std::span<SimulationReport const> reports) const
        {
            return m_Output->slurpValuesFloat(component, reports);
        }

        Vec2 getValueVec2(
            const OpenSim::Component& component,
            const SimulationReport& report) const
        {
            return m_Output->getValueVec2(component, report);
        }

        void getValuesVec2(
            const OpenSim::Component& component,
            std::span<SimulationReport const> report,
            std::function<void(Vec2)> const& consumer) const
        {
            m_Output->getValuesVec2(component, report, consumer);
        }

        std::vector<Vec2> slurpValuesVec2(
            const OpenSim::Component& component,
            std::span<SimulationReport const> report) const
        {
            return m_Output->slurpValuesVec2(component, report);
        }

        std::string getValueString(const OpenSim::Component& component, const SimulationReport& report) const
        {
            return m_Output->getValueString(component, report);
        }

        size_t getHash() const
        {
            return m_Output->getHash();
        }

        bool equals(const IOutputExtractor& other) const
        {
            return m_Output->equals(other);
        }

        operator const IOutputExtractor& () const
        {
            return *m_Output;
        }

        const IOutputExtractor& getInner() const
        {
            return *m_Output;
        }

        friend bool operator==(const OutputExtractor& lhs, const OutputExtractor& rhs)
        {
            return *lhs.m_Output == *rhs.m_Output;
        }
    private:
        friend std::string to_string(const OutputExtractor&);
        friend struct std::hash<OutputExtractor>;

        std::shared_ptr<IOutputExtractor const> m_Output;
    };

    template<std::derived_from<IOutputExtractor> ConcreteOutputExtractor, typename... Args>
    requires std::constructible_from<ConcreteOutputExtractor, Args&&...>
    OutputExtractor make_output_extractor(Args&&... args)
    {
        return OutputExtractor{ConcreteOutputExtractor{std::forward<Args>(args)...}};
    }

    std::ostream& operator<<(std::ostream&, const OutputExtractor&);
    std::string to_string(const OutputExtractor&);
}

template<>
struct std::hash<osc::OutputExtractor> final {
    size_t operator()(const osc::OutputExtractor& o) const
    {
        return o.m_Output->getHash();
    }
};

#pragma once

#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/OutputValueExtractor.h>

#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace osc { class ISimulationState; }
namespace osc { class SimulationReportSequence; }

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

        OutputValueExtractor getOutputValueExtractor(OpenSim::Component const& component) const
        {
            return m_Output->getOutputValueExtractor(component);
        }

        float getValueFloat(
            OpenSim::Component const& component,
            ISimulationState const& state) const
        {
            return m_Output->getValueFloat(component, state);
        }

        void getValuesFloat(
            OpenSim::Model const& model,
            SimulationReportSequence const& reports,
            std::function<void(float)> const& consumer) const
        {
            m_Output->getValuesFloat(model, reports, consumer);
        }

        std::vector<float> slurpValuesFloat(
            OpenSim::Model const& model,
            SimulationReportSequence const& reports) const
        {
            return m_Output->slurpValuesFloat(model, reports);
        }

        Vec2 getValueVec2(
            OpenSim::Component const& component,
            ISimulationState const& report) const
        {
            return m_Output->getValueVec2(component, report);
        }

        void getValuesVec2(
            OpenSim::Model const& model,
            SimulationReportSequence const& reports,
            std::function<void(Vec2)> const& consumer) const
        {
            m_Output->getValuesVec2(model, reports, consumer);
        }

        std::vector<Vec2> slurpValuesVec2(
            OpenSim::Model const& model,
            SimulationReportSequence const& reports) const
        {
            return m_Output->slurpValuesVec2(model, reports);
        }

        std::string getValueString(
            OpenSim::Component const& component,
            ISimulationState const& state) const
        {
            return m_Output->getValueString(component, state);
        }

        size_t getHash() const
        {
            return m_Output->getHash();
        }

        bool equals(IOutputExtractor const& other) const
        {
            return m_Output->equals(other);
        }

        operator IOutputExtractor const& () const
        {
            return *m_Output;
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

        std::shared_ptr<IOutputExtractor const> m_Output;
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

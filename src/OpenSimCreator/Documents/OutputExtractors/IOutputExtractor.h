#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace OpenSim { class Component; }
namespace osc { class IOutputValueExtractorVisitor; }
namespace osc { class SimulationReport; }

namespace osc
{
    // an interface for something that can produce an output value extractor
    // for a particular model against multiple states
    //
    // implementors of this interface are assumed to be immutable (important,
    // because output extractors might be shared between simulations, threads,
    // etc.)
    class IOutputExtractor {
    protected:
        IOutputExtractor() = default;
        IOutputExtractor(IOutputExtractor const&) = default;
        IOutputExtractor(IOutputExtractor&&) noexcept = default;
        IOutputExtractor& operator=(IOutputExtractor const&) = default;
        IOutputExtractor& operator=(IOutputExtractor&&) noexcept = default;
    public:
        virtual ~IOutputExtractor() noexcept = default;

        CStringView getName() const { return implGetName(); }
        CStringView getDescription() const { return implGetDescription(); }

        OutputExtractorDataType getOutputType() const { return implGetOutputType(); }
        OutputValueExtractor getOutputValueExtractor(OpenSim::Component const& component) const
        {
            return implGetOutputValueExtractor(component);
        }

        float getValueFloat(
            OpenSim::Component const&,
            SimulationReport const&
        ) const;

        void getValuesFloat(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::function<void(float)> const& consumer
        ) const;

        std::vector<float> slurpValuesFloat(
            OpenSim::Component const&,
            std::span<SimulationReport const>
        ) const;

        Vec2 getValueVec2(
            OpenSim::Component const& component,
            SimulationReport const& report
        ) const;

        void getValuesVec2(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::function<void(Vec2)> const& consumer
        ) const;

        std::vector<Vec2> slurpValuesVec2(
            OpenSim::Component const&,
            std::span<SimulationReport const>
        ) const;

        std::string getValueString(
            OpenSim::Component const&,
            SimulationReport const&
        ) const;

        size_t getHash() const { return implGetHash(); }
        bool equals(IOutputExtractor const& other) const { return implEquals(other); }

        friend bool operator==(IOutputExtractor const& lhs, IOutputExtractor const& rhs)
        {
            return lhs.equals(rhs);
        }
    private:
        virtual CStringView implGetName() const = 0;
        virtual CStringView implGetDescription() const = 0;
        virtual OutputExtractorDataType implGetOutputType() const = 0;
        virtual OutputValueExtractor implGetOutputValueExtractor(OpenSim::Component const&) const = 0;
        virtual size_t implGetHash() const = 0;
        virtual bool implEquals(IOutputExtractor const&) const = 0;
    };
}

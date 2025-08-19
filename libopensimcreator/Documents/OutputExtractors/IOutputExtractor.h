#pragma once

#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <liboscar/Maths/Vector2.h>
#include <liboscar/Utils/CStringView.h>

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
        IOutputExtractor(const IOutputExtractor&) = default;
        IOutputExtractor(IOutputExtractor&&) noexcept = default;
        IOutputExtractor& operator=(const IOutputExtractor&) = default;
        IOutputExtractor& operator=(IOutputExtractor&&) noexcept = default;
    public:
        virtual ~IOutputExtractor() noexcept = default;

        CStringView getName() const { return implGetName(); }
        CStringView getDescription() const { return implGetDescription(); }

        OutputExtractorDataType getOutputType() const { return implGetOutputType(); }
        OutputValueExtractor getOutputValueExtractor(const OpenSim::Component& component) const
        {
            return implGetOutputValueExtractor(component);
        }

        float getValueFloat(
            const OpenSim::Component&,
            const SimulationReport&
        ) const;

        void getValuesFloat(
            const OpenSim::Component&,
            std::span<const SimulationReport>,
            const std::function<void(float)>& consumer
        ) const;

        std::vector<float> slurpValuesFloat(
            const OpenSim::Component&,
            std::span<const SimulationReport>
        ) const;

        Vector2 getValueVector2(
            const OpenSim::Component& component,
            const SimulationReport& report
        ) const;

        void getValuesVector2(
            const OpenSim::Component&,
            std::span<const SimulationReport>,
            const std::function<void(Vector2)>& consumer
        ) const;

        std::vector<Vector2> slurpValuesVector2(
            const OpenSim::Component&,
            std::span<const SimulationReport>
        ) const;

        std::string getValueString(
            const OpenSim::Component&,
            const SimulationReport&
        ) const;

        size_t getHash() const { return implGetHash(); }
        bool equals(const IOutputExtractor& other) const { return implEquals(other); }

        friend bool operator==(const IOutputExtractor& lhs, const IOutputExtractor& rhs)
        {
            return lhs.equals(rhs);
        }
    private:
        virtual CStringView implGetName() const = 0;
        virtual CStringView implGetDescription() const = 0;
        virtual OutputExtractorDataType implGetOutputType() const = 0;
        virtual OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const = 0;
        virtual size_t implGetHash() const = 0;
        virtual bool implEquals(const IOutputExtractor&) const = 0;
    };
}

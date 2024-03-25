#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <functional>
#include <span>

namespace OpenSim { class Component; }

namespace osc
{
    // an output value extractor that extracts floats
    class IFloatOutputValueExtractor {
    protected:
        IFloatOutputValueExtractor() = default;
        IFloatOutputValueExtractor(IFloatOutputValueExtractor const&) = default;
        IFloatOutputValueExtractor(IFloatOutputValueExtractor&&) noexcept = default;
        IFloatOutputValueExtractor& operator=(IFloatOutputValueExtractor const&) = default;
        IFloatOutputValueExtractor& operator=(IFloatOutputValueExtractor&&) noexcept = default;
    public:
        virtual ~IFloatOutputValueExtractor() noexcept = default;

        void extractFloats(
            OpenSim::Component const& component,
            std::span<SimulationReport const> reports,
            std::function<void(float)> const& consumer) const
        {
            implExtractFloats(component, reports, consumer);
        }

    private:
        virtual void implExtractFloats(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::function<void(float)> const& consumer
        ) const = 0;
    };
}

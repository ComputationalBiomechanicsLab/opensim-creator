#pragma once

#include <string>

namespace osc { class SimulationReport; }
namespace OpenSim { class Component; }

namespace osc
{
    // an output value extractor that extracts strings
    class IStringOutputValueExtractor {
    protected:
        IStringOutputValueExtractor() = default;
        IStringOutputValueExtractor(IStringOutputValueExtractor const&) = default;
        IStringOutputValueExtractor(IStringOutputValueExtractor&&) noexcept = default;
        IStringOutputValueExtractor& operator=(IStringOutputValueExtractor const&) = default;
        IStringOutputValueExtractor& operator=(IStringOutputValueExtractor&&) noexcept = default;
    public:
        virtual ~IStringOutputValueExtractor() noexcept = default;

        std::string extractString(
            OpenSim::Component const& component,
            SimulationReport const& report) const
        {
            return implExtractString(component, report);
        }
    private:
        virtual std::string implExtractString(OpenSim::Component const&, SimulationReport const&) const = 0;
    };
}

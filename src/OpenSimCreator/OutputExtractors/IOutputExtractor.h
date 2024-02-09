#pragma once

#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>
#include <string>

namespace OpenSim { class Component; }
namespace osc { class SimulationReport; }

namespace osc
{
    // indicates the datatype that the output extractor emits
    enum class OutputType {
        Float = 0,
        String,
    };

    // interface for something that can extract data from simulation reports
    //
    // assumed to be an immutable type (important, because output extractors
    // might be shared between simulations, threads, etc.) that merely extracts
    // data from simulation reports
    class IOutputExtractor {
    protected:
        IOutputExtractor() = default;
        IOutputExtractor(IOutputExtractor const&) = default;
        IOutputExtractor(IOutputExtractor&&) noexcept = default;
        IOutputExtractor& operator=(IOutputExtractor const&) = default;
        IOutputExtractor& operator=(IOutputExtractor&&) noexcept = default;
    public:
        virtual ~IOutputExtractor() noexcept = default;

        virtual CStringView getName() const = 0;
        virtual CStringView getDescription() const = 0;

        virtual OutputType getOutputType() const = 0;

        virtual float getValueFloat(
            OpenSim::Component const&,
            SimulationReport const&
        ) const = 0;

        virtual void getValuesFloat(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::span<float> overwriteOut
        ) const = 0;

        virtual std::string getValueString(
            OpenSim::Component const&,
            SimulationReport const&
        ) const = 0;

        virtual size_t getHash() const = 0;
        virtual bool equals(IOutputExtractor const&) const = 0;

        friend bool operator==(IOutputExtractor const& lhs, IOutputExtractor const& rhs)
        {
            return lhs.equals(rhs);
        }
    };
}

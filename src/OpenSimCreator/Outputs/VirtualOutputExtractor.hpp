#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
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
    class VirtualOutputExtractor {
    protected:
        VirtualOutputExtractor() = default;
        VirtualOutputExtractor(VirtualOutputExtractor const&) = default;
        VirtualOutputExtractor(VirtualOutputExtractor&&) noexcept = default;
        VirtualOutputExtractor& operator=(VirtualOutputExtractor const&) = default;
        VirtualOutputExtractor& operator=(VirtualOutputExtractor&&) noexcept = default;
    public:
        virtual ~VirtualOutputExtractor() noexcept = default;

        virtual CStringView getName() const = 0;
        virtual CStringView getDescription() const = 0;

        virtual OutputType getOutputType() const = 0;

        virtual float getValueFloat(
            OpenSim::Component const&,
            SimulationReport const&
        ) const = 0;

        virtual void getValuesFloat(
            OpenSim::Component const&,
            nonstd::span<SimulationReport const>,
            nonstd::span<float> overwriteOut
        ) const = 0;

        virtual std::string getValueString(
            OpenSim::Component const&,
            SimulationReport const&
        ) const = 0;

        virtual size_t getHash() const = 0;
        virtual bool equals(VirtualOutputExtractor const&) const = 0;
    };


    inline bool operator==(VirtualOutputExtractor const& a, VirtualOutputExtractor const& b)
    {
        return a.equals(b);
    }

    inline bool operator!=(VirtualOutputExtractor const& a, VirtualOutputExtractor const& b)
    {
        return !a.equals(b);
    }
}

#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractorDataType.h>

#include <oscar/Utils/CStringView.h>

#include <array>
#include <cstddef>
#include <span>
#include <string>

namespace OpenSim { class Component; }

namespace osc
{
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

        CStringView getName() const { return implGetName(); }
        CStringView getDescription() const { return implGetDescription(); }

        OutputExtractorDataType getOutputType() const { return implGetOutputType(); }
        float getValueFloat(
            OpenSim::Component const& component,
            SimulationReport const& report) const
        {
            std::span<SimulationReport const> reports(&report, 1);
            std::array<float, 1> out{};
            implGetValuesFloat(component, reports, out);
            return out.front();
        }

        void getValuesFloat(
            OpenSim::Component const& component,
            std::span<SimulationReport const> reports,
            std::span<float> overwriteOut) const
        {
            implGetValuesFloat(component, reports, overwriteOut);
        }

        std::string getValueString(
            OpenSim::Component const& component,
            SimulationReport const& report) const
        {
            return implGetValueString(component, report);
        }

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
        virtual void implGetValuesFloat(OpenSim::Component const&, std::span<SimulationReport const>, std::span<float> overwriteOut) const = 0;
        virtual std::string implGetValueString(OpenSim::Component const&, SimulationReport const&) const = 0;

        virtual size_t implGetHash() const = 0;
        virtual bool implEquals(IOutputExtractor const&) const = 0;
    };
}

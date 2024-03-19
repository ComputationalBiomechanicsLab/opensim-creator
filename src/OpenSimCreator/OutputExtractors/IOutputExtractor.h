#pragma once

#include <OpenSimCreator/OutputExtractors/OutputExtractorDataType.h>

#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>
#include <string>

namespace OpenSim { class Component; }
namespace osc { class IOutputValueExtractorVisitor; }
namespace osc { class SimulationReport; }

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

        OutputExtractorDataType getOutputType() const;

        float getValueFloat(
            OpenSim::Component const&,
            SimulationReport const&
        ) const;

        void getValuesFloat(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::span<float> overwriteOut
        ) const;

        Vec2 getValueVec2(
            OpenSim::Component const& component,
            SimulationReport const& report
        ) const;

        void getValuesVec2(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::span<Vec2> overwriteOut
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
        virtual void implAccept(IOutputValueExtractorVisitor&) const = 0;
        virtual size_t implGetHash() const = 0;
        virtual bool implEquals(IOutputExtractor const&) const = 0;
    };
}

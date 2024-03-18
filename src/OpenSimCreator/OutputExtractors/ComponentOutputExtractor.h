#pragma once

#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ClonePtr.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class SimulationReport; }

namespace osc
{
    // flag type that can be used to say what subfields an OpenSim output has
    enum class OutputSubfield : uint32_t {
        None      = 0,
        X         = 1<<0,
        Y         = 1<<1,
        Z         = 1<<2,
        Magnitude = 1<<3,

        Default = None,
    };

    constexpr OutputSubfield operator|(OutputSubfield lhs, OutputSubfield rhs)
    {
        return static_cast<OutputSubfield>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr bool operator&(OutputSubfield lhs, OutputSubfield rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }

    std::optional<CStringView> GetOutputSubfieldLabel(OutputSubfield);
    std::span<OutputSubfield const> GetAllSupportedOutputSubfields();

    // returns applicable OutputSubfield ORed together
    OutputSubfield GetSupportedSubfields(OpenSim::AbstractOutput const&);

    // an output extractor that uses the `OpenSim::AbstractOutput` API to extract a value
    // from a component
    class ComponentOutputExtractor final : public IOutputExtractor {
    public:
        ComponentOutputExtractor(
            OpenSim::AbstractOutput const&,
            OutputSubfield = OutputSubfield::None
        );
        ComponentOutputExtractor(ComponentOutputExtractor const&);
        ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept;
        ComponentOutputExtractor& operator=(ComponentOutputExtractor const&);
        ComponentOutputExtractor& operator=(ComponentOutputExtractor&&) noexcept;
        ~ComponentOutputExtractor() noexcept override;

        OpenSim::ComponentPath const& getComponentAbsPath() const;

    private:
        CStringView implGetName() const final;
        CStringView implGetDescription() const final;

        OutputExtractorDataType implGetOutputType() const final;

        void implGetValuesFloat(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::span<float> overwriteOut
        ) const final;

        std::string implGetValueString(
            OpenSim::Component const&,
            SimulationReport const&
        ) const final;

        size_t implGetHash() const final;
        bool implEquals(IOutputExtractor const&) const final;

    private:
        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}

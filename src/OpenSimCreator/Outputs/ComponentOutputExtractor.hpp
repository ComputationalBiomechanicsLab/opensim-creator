#pragma once

#include "OpenSimCreator/Outputs/VirtualOutputExtractor.hpp"

#include <oscar/Utils/ClonePtr.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

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

    constexpr OutputSubfield operator|(OutputSubfield a, OutputSubfield b) noexcept
    {
        using Ut = std::underlying_type_t<OutputSubfield>;
        return static_cast<OutputSubfield>(static_cast<Ut>(a) | static_cast<Ut>(b));
    }

    constexpr bool operator&(OutputSubfield a, OutputSubfield b) noexcept
    {
        using Ut = std::underlying_type_t<OutputSubfield>;
        return (static_cast<Ut>(a) & static_cast<Ut>(b)) != 0;
    }

    std::optional<CStringView> GetOutputSubfieldLabel(OutputSubfield);
    nonstd::span<OutputSubfield const> GetAllSupportedOutputSubfields();

    // returns applicable OutputSubfield ORed together
    OutputSubfield GetSupportedSubfields(OpenSim::AbstractOutput const&);

    // an output extractor that uses the `OpenSim::AbstractOutput` API to extract a value
    // from a component
    class ComponentOutputExtractor final : public VirtualOutputExtractor {
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
        CStringView getName() const final;
        CStringView getDescription() const final;

        OutputType getOutputType() const final;

        float getValueFloat(
            OpenSim::Component const&,
            SimulationReport const&
        ) const final;

        void getValuesFloat(
            OpenSim::Component const&,
            nonstd::span<SimulationReport const>,
            nonstd::span<float> overwriteOut
        ) const final;

        std::string getValueString(
            OpenSim::Component const&,
            SimulationReport const&
        ) const final;

        size_t getHash() const final;
        bool equals(VirtualOutputExtractor const&) const final;

    private:
        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}

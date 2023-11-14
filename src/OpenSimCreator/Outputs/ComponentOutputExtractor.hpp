#pragma once

#include <OpenSimCreator/Outputs/VirtualOutputExtractor.hpp>

#include <oscar/Shims/Cpp23/utility.hpp>
#include <oscar/Utils/ClonePtr.hpp>
#include <oscar/Utils/CStringView.hpp>

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

    constexpr OutputSubfield operator|(OutputSubfield a, OutputSubfield b) noexcept
    {
        return static_cast<OutputSubfield>(osc::to_underlying(a) | osc::to_underlying(b));
    }

    constexpr bool operator&(OutputSubfield a, OutputSubfield b) noexcept
    {
        return (osc::to_underlying(a) & osc::to_underlying(b)) != 0;
    }

    std::optional<CStringView> GetOutputSubfieldLabel(OutputSubfield);
    std::span<OutputSubfield const> GetAllSupportedOutputSubfields();

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
            std::span<SimulationReport const>,
            std::span<float> overwriteOut
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

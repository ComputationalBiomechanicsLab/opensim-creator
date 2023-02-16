#pragma once

#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"
#include "src/Utils/ClonePtr.hpp"

#include <nonstd/span.hpp>

#include <cstddef>
#include <string>

namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class SimulationReport; }

namespace osc
{
    // flag type that can be used to say what subfields an OpenSim output has
    enum class OutputSubfield {
        None = 0,
        X = 1<<0,
        Y = 1<<1,
        Z = 1<<2,
        Magnitude = 1<<3,
        Default = None,
    };

    char const* GetOutputSubfieldLabel(OutputSubfield);
    nonstd::span<OutputSubfield const> GetAllSupportedOutputSubfields();

    // returns applicable OutputSubfield ORed together
    int GetSupportedSubfields(OpenSim::AbstractOutput const&);

    // an output extractor that uses the `OpenSim::AbstractOutput` API to extract a value
    // from a component
    class ComponentOutputExtractor final : public VirtualOutputExtractor {
    public:
        ComponentOutputExtractor(OpenSim::AbstractOutput const&,
                                 OutputSubfield = OutputSubfield::None);
        ComponentOutputExtractor(ComponentOutputExtractor const&);
        ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept;
        ComponentOutputExtractor& operator=(ComponentOutputExtractor const&);
        ComponentOutputExtractor& operator=(ComponentOutputExtractor&&) noexcept;
        ~ComponentOutputExtractor() noexcept override;

        OpenSim::ComponentPath const& getComponentAbsPath() const;
        std::string const& getName() const final;
        std::string const& getDescription() const final;

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

        std::size_t getHash() const final;
        bool equals(VirtualOutputExtractor const&) const final;

    private:
        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}

#pragma once

#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"
#include "src/Utils/ClonePtr.hpp"

#include <nonstd/span.hpp>

#include <cstddef>
#include <string>


namespace OpenSim
{
    class AbstractOutput;
    class Component;
}

namespace osc
{
    class SimulationReport;
}

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
        ~ComponentOutputExtractor() noexcept;

        std::string const& getName() const override;
        std::string const& getDescription() const override;

        OutputType getOutputType() const override;
        float getValueFloat(OpenSim::Component const&,
                            SimulationReport const&) const override;
        void getValuesFloat(OpenSim::Component const&,
                            nonstd::span<SimulationReport const>,
                            nonstd::span<float> overwriteOut) const override;
        std::string getValueString(OpenSim::Component const&,
                                   SimulationReport const&) const override;

        std::size_t getHash() const override;
        bool equals(VirtualOutputExtractor const&) const override;

    private:
        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}

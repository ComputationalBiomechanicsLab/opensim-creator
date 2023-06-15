#pragma once

#include "OpenSimCreator/Outputs/OutputExtractor.hpp"
#include "OpenSimCreator/Outputs/VirtualOutputExtractor.hpp"

#include <oscar/Utils/UID.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace OpenSim { class Component; }
namespace osc { class SimulationReport; }
namespace SimTK { class Integrator; }

namespace osc
{
    // an output extractor that extracts integrator metadata (e.g. predicted step size)
    class IntegratorOutputExtractor final : public VirtualOutputExtractor {
    public:
        using ExtractorFn = float (*)(SimTK::Integrator const&);

        IntegratorOutputExtractor(
            std::string_view name,
            std::string_view description,
            ExtractorFn extractor
        );

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

        UID getAuxiliaryDataID() const;
        ExtractorFn getExtractorFunction() const;

    private:
        UID m_AuxiliaryDataID;
        std::string m_Name;
        std::string m_Description;
        ExtractorFn m_Extractor;
    };

    int GetNumIntegratorOutputExtractors();
    IntegratorOutputExtractor const& GetIntegratorOutputExtractor(int idx);
    OutputExtractor GetIntegratorOutputExtractorDynamic(int idx);
}

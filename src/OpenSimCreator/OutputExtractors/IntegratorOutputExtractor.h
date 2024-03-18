#pragma once

#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractor.h>

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace OpenSim { class Component; }
namespace osc { class SimulationReport; }
namespace SimTK { class Integrator; }

namespace osc
{
    // an output extractor that extracts integrator metadata (e.g. predicted step size)
    class IntegratorOutputExtractor final : public IOutputExtractor {
    public:
        using ExtractorFn = float (*)(SimTK::Integrator const&);

        IntegratorOutputExtractor(
            std::string_view name,
            std::string_view description,
            ExtractorFn extractor
        );

        UID getAuxiliaryDataID() const;
        ExtractorFn getExtractorFunction() const;

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

        UID m_AuxiliaryDataID;
        std::string m_Name;
        std::string m_Description;
        ExtractorFn m_Extractor;
    };

    int GetNumIntegratorOutputExtractors();
    IntegratorOutputExtractor const& GetIntegratorOutputExtractor(int idx);
    OutputExtractor GetIntegratorOutputExtractorDynamic(int idx);
}

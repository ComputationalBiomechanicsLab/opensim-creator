#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace OpenSim { class Component; }
namespace SimTK { class Integrator; }

namespace osc
{
    // an output extractor that extracts integrator metadata (e.g. predicted step size)
    class IntegratorOutputExtractor final : public IOutputExtractor {
    public:
        using ExtractorFn = float (*)(const SimTK::Integrator&);

        IntegratorOutputExtractor(
            std::string_view name,
            std::string_view description,
            ExtractorFn extractor) :

            m_Name{name},
            m_Description{description},
            m_Extractor{extractor}
        {}

        UID getAuxiliaryDataID() const { return m_AuxiliaryDataID; }
        ExtractorFn getExtractorFunction() const { return m_Extractor; }

    private:
        CStringView implGetName() const final { return m_Name; }
        CStringView implGetDescription() const final { return m_Description; }
        OutputExtractorDataType implGetOutputType() const override { return OutputExtractorDataType::Float; }
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const final;
        size_t implGetHash() const final;
        bool implEquals(const IOutputExtractor&) const final;

        UID m_AuxiliaryDataID;
        std::string m_Name;
        std::string m_Description;
        ExtractorFn m_Extractor;
    };

    int GetNumIntegratorOutputExtractors();
    const IntegratorOutputExtractor& GetIntegratorOutputExtractor(int idx);
    OutputExtractor GetIntegratorOutputExtractorDynamic(int idx);
}

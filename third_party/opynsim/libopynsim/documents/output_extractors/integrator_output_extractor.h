#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace OpenSim { class Component; }
namespace SimTK { class Integrator; }

namespace opyn
{
    // an output extractor that extracts integrator metadata (e.g. predicted step size)
    class IntegratorOutputExtractor final : public OutputExtractor {
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

        osc::UID getAuxiliaryDataID() const { return m_AuxiliaryDataID; }
        ExtractorFn getExtractorFunction() const { return m_Extractor; }

    private:
        osc::CStringView implGetName() const final { return m_Name; }
        osc::CStringView implGetDescription() const final { return m_Description; }
        OutputExtractorDataType implGetOutputType() const override { return OutputExtractorDataType::Float; }
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const final;
        size_t implGetHash() const final;
        bool implEquals(const OutputExtractor&) const final;

        osc::UID m_AuxiliaryDataID;
        std::string m_Name;
        std::string m_Description;
        ExtractorFn m_Extractor;
    };

    int GetNumIntegratorOutputExtractors();
    const IntegratorOutputExtractor& GetIntegratorOutputExtractor(int idx);
    SharedOutputExtractor GetIntegratorOutputExtractorDynamic(int idx);
}

#pragma once

#include <libopensimcreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/UID.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace OpenSim { class Component; }
namespace SimTK { class MultibodySystem; }

namespace osc
{
    // an output extractor that uses a free-function to extract a single value from
    // a SimTK::MultiBodySystem
    //
    // handy for extracting simulation stats (e.g. num steps taken etc.)
    class MultiBodySystemOutputExtractor final : public IOutputExtractor {
    public:
        using ExtractorFn = float (*)(const SimTK::MultibodySystem&);

        MultiBodySystemOutputExtractor(
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
        OutputExtractorDataType implGetOutputType() const final { return OutputExtractorDataType::Float; }
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const final;
        size_t implGetHash() const final;
        bool implEquals(const IOutputExtractor&) const final;

        UID m_AuxiliaryDataID;
        std::string m_Name;
        std::string m_Description;
        ExtractorFn m_Extractor;
    };

    int GetNumMultiBodySystemOutputExtractors();
    const MultiBodySystemOutputExtractor& GetMultiBodySystemOutputExtractor(int idx);
    OutputExtractor GetMultiBodySystemOutputExtractorDynamic(int idx);
}

#pragma once

#include <libopensimcreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <liboscar/utils/CStringView.h>

#include <cstddef>
#include <string>

namespace OpenSim { class Component; }

namespace osc
{
    // an output extractor that concatenates the outputs from multiple output extractors
    class ConcatenatingOutputExtractor final : public IOutputExtractor {
    public:
        ConcatenatingOutputExtractor(OutputExtractor first_, OutputExtractor second_);

    private:
        CStringView implGetName() const override { return m_Label; }
        CStringView implGetDescription() const override { return {}; }
        OutputExtractorDataType implGetOutputType() const override { return m_OutputType; }
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const override;
        size_t implGetHash() const override;
        bool implEquals(const IOutputExtractor&) const override;

        OutputExtractor m_First;
        OutputExtractor m_Second;
        OutputExtractorDataType m_OutputType;
        std::string m_Label;
    };
}

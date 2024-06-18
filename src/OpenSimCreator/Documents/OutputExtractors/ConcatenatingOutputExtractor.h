#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <oscar/Utils/CStringView.h>

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

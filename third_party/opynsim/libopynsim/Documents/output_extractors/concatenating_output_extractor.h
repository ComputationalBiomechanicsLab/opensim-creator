#pragma once

#include <libopynsim/Documents/output_extractors/output_extractor.h>
#include <libopynsim/Documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/Documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/Documents/output_extractors/output_value_extractor.h>

#include <liboscar/utils/c_string_view.h>

#include <cstddef>
#include <string>

namespace OpenSim { class Component; }

namespace osc
{
    // an output extractor that concatenates the outputs from multiple output extractors
    class ConcatenatingOutputExtractor final : public OutputExtractor {
    public:
        ConcatenatingOutputExtractor(SharedOutputExtractor first_, SharedOutputExtractor second_);

    private:
        CStringView implGetName() const override { return m_Label; }
        CStringView implGetDescription() const override { return {}; }
        OutputExtractorDataType implGetOutputType() const override { return m_OutputType; }
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const override;
        size_t implGetHash() const override;
        bool implEquals(const OutputExtractor&) const override;

        SharedOutputExtractor m_First;
        SharedOutputExtractor m_Second;
        OutputExtractorDataType m_OutputType;
        std::string m_Label;
    };
}

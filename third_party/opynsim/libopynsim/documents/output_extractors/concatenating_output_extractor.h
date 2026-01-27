#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/c_string_view.h>

#include <cstddef>
#include <string>

namespace OpenSim { class Component; }

namespace osc
{
    // an output extractor that concatenates the outputs from multiple output extractors
    class ConcatenatingOutputExtractor final : public OutputExtractor {
    public:
        ConcatenatingOutputExtractor(opyn::SharedOutputExtractor first_, opyn::SharedOutputExtractor second_);

    private:
        CStringView implGetName() const override { return m_Label; }
        CStringView implGetDescription() const override { return {}; }
        opyn::OutputExtractorDataType implGetOutputType() const override { return m_OutputType; }
        opyn::OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const override;
        size_t implGetHash() const override;
        bool implEquals(const OutputExtractor&) const override;

        opyn::SharedOutputExtractor m_First;
        opyn::SharedOutputExtractor m_Second;
        opyn::OutputExtractorDataType m_OutputType;
        std::string m_Label;
    };
}

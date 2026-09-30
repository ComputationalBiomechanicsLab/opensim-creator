#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/c_string_view.h>

#include <cstddef>
#include <string>

namespace OpenSim { class Component; }

namespace opyn
{
    // an output extractor that concatenates the outputs from multiple output extractors
    class ConcatenatingOutputExtractor final : public OutputExtractor {
    public:
        ConcatenatingOutputExtractor(SharedOutputExtractor first, SharedOutputExtractor second);

    private:
        osc::CStringView impl_name() const override { return label_; }
        osc::CStringView impl_description() const override { return {}; }
        OutputExtractorDataType impl_output_type() const override { return output_type_; }
        OutputValueExtractor impl_output_value_extractor(const OpenSim::Component&) const override;
        size_t impl_hash() const override;
        bool impl_equals(const OutputExtractor&) const override;

        SharedOutputExtractor first_;
        SharedOutputExtractor second_;
        OutputExtractorDataType output_type_;
        std::string label_;
    };
}

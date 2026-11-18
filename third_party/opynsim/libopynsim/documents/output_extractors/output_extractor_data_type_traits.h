#pragma once

#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>

namespace opyn
{
    template<OutputExtractorDataType>
    struct OutputExtractorDataTypeTraits;

    template<>
    struct OutputExtractorDataTypeTraits<OutputExtractorDataType::Float> {
        static constexpr bool is_numeric = true;
    };

    template<>
    struct OutputExtractorDataTypeTraits<OutputExtractorDataType::Vector2> {
        static constexpr bool is_numeric = true;
    };

    template<>
    struct OutputExtractorDataTypeTraits<OutputExtractorDataType::String> {
        static constexpr bool is_numeric = false;
    };
}

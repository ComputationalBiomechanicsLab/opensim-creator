#pragma once

#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>

#include <liboscar/utils/enum_helpers.h>

namespace osc
{
    using OutputExtractorDataTypeList = OptionList<OutputExtractorDataType,
        OutputExtractorDataType::Float,
        OutputExtractorDataType::Vector2,
        OutputExtractorDataType::String
    >;
}

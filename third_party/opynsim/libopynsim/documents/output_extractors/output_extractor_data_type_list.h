#pragma once

#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>

#include <liboscar/utilities/enum_helpers.h>

namespace opyn
{
    using OutputExtractorDataTypeList = osc::OptionList<OutputExtractorDataType,
        OutputExtractorDataType::Float,
        OutputExtractorDataType::Vector2,
        OutputExtractorDataType::String
    >;
}

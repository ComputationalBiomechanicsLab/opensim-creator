#pragma once

#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataType.h>

#include <liboscar/utils/enum_helpers.h>

namespace osc
{
    using OutputExtractorDataTypeList = OptionList<OutputExtractorDataType,
        OutputExtractorDataType::Float,
        OutputExtractorDataType::Vector2,
        OutputExtractorDataType::String
    >;
}

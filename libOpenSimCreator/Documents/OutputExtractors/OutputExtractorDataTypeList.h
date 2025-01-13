#pragma once

#include <libOpenSimCreator/Documents/OutputExtractors/OutputExtractorDataType.h>

#include <liboscar/Utils/EnumHelpers.h>

namespace osc
{
    using OutputExtractorDataTypeList = OptionList<OutputExtractorDataType,
        OutputExtractorDataType::Float,
        OutputExtractorDataType::Vec2,
        OutputExtractorDataType::String
    >;
}

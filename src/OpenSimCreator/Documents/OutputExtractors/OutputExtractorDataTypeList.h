#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataType.h>

#include <oscar/Utils/EnumHelpers.h>

namespace osc
{
    using OutputExtractorDataTypeList = OptionList<OutputExtractorDataType,
        OutputExtractorDataType::Float,
        OutputExtractorDataType::Vec2,
        OutputExtractorDataType::String
    >;
}

#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataTypeList.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataTypeTraits.h>

#include <oscar/Utils/EnumHelpers.h>

#include <array>

namespace osc
{
    constexpr bool is_numeric(OutputExtractorDataType t)
    {
        constexpr auto lut = []<OutputExtractorDataType... Types>(OptionList<OutputExtractorDataType, Types...>) {
            return std::to_array({ OutputExtractorDataTypeTraits<Types>::is_numeric... });
        }(OutputExtractorDataTypeList{});

        return lut.at(to_index(t));
    }
}

#pragma once

#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataTypeList.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataTypeTraits.h>

#include <liboscar/Utils/EnumHelpers.h>

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

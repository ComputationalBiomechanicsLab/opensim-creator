#pragma once

#include <libopynsim/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <libopynsim/Documents/OutputExtractors/OutputExtractorDataTypeList.h>
#include <libopynsim/Documents/OutputExtractors/OutputExtractorDataTypeTraits.h>

#include <liboscar/utils/enum_helpers.h>

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

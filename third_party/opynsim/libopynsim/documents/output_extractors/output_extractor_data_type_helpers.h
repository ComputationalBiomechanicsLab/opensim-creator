#pragma once

#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type_list.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type_traits.h>

#include <liboscar/utilities/enum_helpers.h>

#include <array>

namespace opyn
{
    constexpr bool is_numeric(OutputExtractorDataType t)
    {
        constexpr auto lut = []<OutputExtractorDataType... Types>(osc::OptionList<OutputExtractorDataType, Types...>) {
            return std::to_array({ OutputExtractorDataTypeTraits<Types>::is_numeric... });
        }(OutputExtractorDataTypeList{});

        return lut.at(osc::to_index(t));
    }
}

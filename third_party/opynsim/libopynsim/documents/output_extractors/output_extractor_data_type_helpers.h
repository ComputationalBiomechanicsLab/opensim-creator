#pragma once

#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type_list.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type_traits.h>

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

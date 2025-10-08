#pragma once

#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataType.h>

namespace osc
{
    template<OutputExtractorDataType>
    struct OutputExtractorDataTypeTraits;

    template<>
    struct OutputExtractorDataTypeTraits<OutputExtractorDataType::Float> {
        static inline constexpr bool is_numeric = true;
    };

    template<>
    struct OutputExtractorDataTypeTraits<OutputExtractorDataType::Vector2> {
        static inline constexpr bool is_numeric = true;
    };

    template<>
    struct OutputExtractorDataTypeTraits<OutputExtractorDataType::String> {
        static inline constexpr bool is_numeric = false;
    };
}

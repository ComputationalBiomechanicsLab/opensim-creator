#pragma once

#include <oscar/Utils/EnumHelpers.h>

namespace osc::detail
{
    // used by the texture implementation to keep track of what kind of
    // data it is storing
    enum class CPUDataType {
        UnsignedByte,
        Float,
        UnsignedInt24_8,
        HalfFloat,
        NUM_OPTIONS,
    };

    using CPUDataTypeList = OptionList<CPUDataType,
        CPUDataType::UnsignedByte,
        CPUDataType::Float,
        CPUDataType::UnsignedInt24_8,
        CPUDataType::HalfFloat
    >;
}

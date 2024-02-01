#pragma once

#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/NonTypelist.hpp>

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

    using CPUDataTypeList = NonTypelist<CPUDataType,
        CPUDataType::UnsignedByte,
        CPUDataType::Float,
        CPUDataType::UnsignedInt24_8,
        CPUDataType::HalfFloat
    >;
    static_assert(NumOptions<CPUDataType>() == NonTypelistSizeV<CPUDataTypeList>);
}

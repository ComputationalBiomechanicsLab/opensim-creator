#include "VariantType.h"

#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar/Variant/VariantTypeList.h>
#include <oscar/Variant/VariantTypeTraits.h>

#include <array>
#include <cstddef>
#include <ostream>
#include <string>

std::ostream& osc::operator<<(std::ostream& out, VariantType variant_type)
{
    constexpr auto lut = []<VariantType... Types>(OptionList<VariantType, Types...>)
    {
        return std::to_array({ VariantTypeTraits<Types>::name... });
    }(VariantTypeList{});

    return out << lut.at(to_index(variant_type));
}

std::string osc::to_string(VariantType variant_type)
{
    return stream_to_string(variant_type);
}

#include "VariantType.h"

#include <liboscar/utils/EnumHelpers.h>
#include <liboscar/variant/VariantTypeList.h>
#include <liboscar/variant/VariantTypeTraits.h>

#include <array>
#include <ostream>

std::ostream& osc::operator<<(std::ostream& out, VariantType variant_type)
{
    constexpr auto lut = []<VariantType... Types>(OptionList<VariantType, Types...>)
    {
        return std::to_array({ VariantTypeTraits<Types>::name... });
    }(VariantTypeList{});

    return out << lut.at(to_index(variant_type));
}

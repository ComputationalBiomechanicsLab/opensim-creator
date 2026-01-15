#include "variant_type.h"

#include <liboscar/utils/EnumHelpers.h>
#include <liboscar/variant/variant_type_list.h>
#include <liboscar/variant/variant_type_traits.h>

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

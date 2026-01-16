#pragma once

#include <liboscar/graphics/material_property_value_types.h>
#include <liboscar/utils/typelist.h>

namespace osc
{
    template<typename T>
    concept MaterialPropertyValue = MaterialPropertyValueTypes::contains<T>();
}

#pragma once

#include <liboscar/graphics/material_property_value_types.h>
#include <liboscar/utilities/typelist.h>

namespace osc
{
    template<typename T>
    concept MaterialPropertyValue = MaterialPropertyValueTypes::contains<T>();
}

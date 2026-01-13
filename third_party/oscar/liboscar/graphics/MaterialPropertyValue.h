#pragma once

#include <liboscar/graphics/MaterialPropertyValueTypes.h>
#include <liboscar/utils/Typelist.h>

namespace osc
{
    template<typename T>
    concept MaterialPropertyValue = MaterialPropertyValueTypes::contains<T>();
}

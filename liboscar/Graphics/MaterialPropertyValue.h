#pragma once

#include <liboscar/Graphics/MaterialPropertyValueTypes.h>
#include <liboscar/Utils/Typelist.h>

namespace osc
{
    template<typename T>
    concept MaterialPropertyValue = MaterialPropertyValueTypes::contains<T>();
}

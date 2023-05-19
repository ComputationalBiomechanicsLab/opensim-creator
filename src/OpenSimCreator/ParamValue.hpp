#pragma once

#include "OpenSimCreator/IntegratorMethod.hpp"

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, IntegratorMethod>;
}
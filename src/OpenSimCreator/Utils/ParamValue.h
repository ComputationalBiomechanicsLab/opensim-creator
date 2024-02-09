#pragma once

#include <OpenSimCreator/Documents/Simulation/IntegratorMethod.h>

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, IntegratorMethod>;
}

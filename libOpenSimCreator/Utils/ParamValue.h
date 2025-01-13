#pragma once

#include <libOpenSimCreator/Documents/Simulation/IntegratorMethod.h>

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, IntegratorMethod>;
}

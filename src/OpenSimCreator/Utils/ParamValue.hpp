#pragma once

#include "OpenSimCreator/Simulation/IntegratorMethod.hpp"

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, IntegratorMethod>;
}
#pragma once

#include <OpenSimCreator/Documents/Simulation/IntegratorMethod.hpp>

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, IntegratorMethod>;
}

#pragma once

#include <libopensimcreator/Documents/Simulation/IntegratorMethod.h>

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, IntegratorMethod>;
}

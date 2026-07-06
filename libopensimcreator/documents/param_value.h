#pragma once

#include <libopynsim/integrator_method.h>

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, opyn::IntegratorMethod>;
}

#pragma once

#include <libopynsim/solvers/integrator_method.h>

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, opyn::IntegratorMethod>;
}

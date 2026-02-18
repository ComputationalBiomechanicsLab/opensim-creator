#pragma once

#include <libopensimcreator/documents/simulation/integrator_method.h>

#include <variant>

namespace osc
{
    using ParamValue = std::variant<double, int, IntegratorMethod>;
}

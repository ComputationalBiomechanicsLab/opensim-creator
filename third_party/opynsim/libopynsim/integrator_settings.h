#pragma once

#include <libopynsim/integrator_method.h>

#include <cstddef>

namespace opyn
{
    /// Represents the settings of an integrator.
    struct IntegratorSettings final {
        IntegratorMethod method = IntegratorMethod::Default;
        double minimum_step_size = 1.0e-8;
        double maximum_step_size = 1.0;
        double accuracy = 1.0e-5;
        size_t internal_step_limit = 0;
    };
}

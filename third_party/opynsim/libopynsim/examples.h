#pragma once

#include <libopynsim/model.h>
#include <libopynsim/model_specification.h>

namespace opyn::examples
{
    /// Returns a `ModelSpecification` for a simple pendulum.
    ModelSpecification pendulum_specification();

    /// Returns the equivalent of `pendulum_specification().compile()`.
    Model pendulum_model();

    /// Returns a `ModelSpecification` for an example double pendulum.
    ModelSpecification double_pendulum_specification();

    /// Returns the equivalent of `double_pendulum_specification().compile()`.
    Model double_pendulum_model();
}

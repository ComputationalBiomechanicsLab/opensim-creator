#include "examples.h"

#include <libopynsim/model_specification.h>

using namespace opyn;

ModelSpecification opyn::examples::pendulum_specification()
{
    return ModelSpecification::example_pendulum();
}

Model opyn::examples::pendulum_model()
{
    return pendulum_specification().compile();
}

ModelSpecification opyn::examples::double_pendulum_specification()
{
    return ModelSpecification::example_double_pendulum();
}

Model examples::double_pendulum_model()
{
    return double_pendulum_specification().compile();
}

#include "examples.h"

#include <libopynsim/examples.h>
#include <nanobind/nanobind.h>

void opyn::init_examples_submodule(nanobind::module_& examples_module)
{
    examples_module.def(
        "pendulum_specification",
        opyn::examples::pendulum_specification,
        R"(
            Returns a :class:`ModelSpecification` of a pendulum.

            The specification is built entirely in memory with no external data files, which
            makes it useful for debugging, example Python scripts, and documentation pages. The
            returned specification should be for a one kilogram mass suspended one meter away
            from a pin joint that is one meter above the ground. For visualization, the head is
            represented as a sphere with a radius of five centimeters and the suspension rod has
            a radius of five millimeters.
        )"
    );
    examples_module.def(
        "pendulum_model",
        opyn::examples::pendulum_model,
        R"(
            Returns the equivalent of ``opynsim.examples.pendulum_specification().compile()``.

            See :func:`opynsim.examples.pendulum_specification` for a description of the pendulum.
        )"
    );
    examples_module.def(
        "double_pendulum_specification",
        opyn::examples::double_pendulum_specification,
        R"(
            Returns a :class:`ModelSpecification` of a double pendulum.

            The specification is built entirely in-memory with no external data files, which makes
            it useful for debugging, example Python scripts, and documentation pages. The returned
            specification is designed to resemble the ``double_pendulum.osim``, which is available
            as an example file in `OpenSim Creator <https://www.opensimcreator.com>`_ .
        )"
    );
    examples_module.def(
        "double_pendulum_model",
        opyn::examples::double_pendulum_model,
        R"(
            Returns the equivalent of ``opynsim.examples.double_pendulum_specification().compile()``.

            See :func:`opynsim.examples.double_pendulum_specification` for a description of the double pendulum.
        )"
    );
}

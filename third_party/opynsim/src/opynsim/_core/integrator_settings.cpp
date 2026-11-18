#include "integrator_settings.h"

#include <libopynsim/solvers/integrator_settings.h>

#include <nanobind/nanobind.h>

namespace nb = nanobind;

void opyn::python::def_integrator_settings(nb::module_& m)
{
    nb::class_<IntegratorSettings> cls(m, "IntegratorSettings", R"(
        Settings for a forward integrator (e.g. as used by :class:`ForwardDynamicsSolver`).

        **Note**: Modifying integrator settings can have a large effect on its performance
        and behavior. When tweaking the settings, it is recommended to re-validate outcomes.
    )");
    cls.def(nb::init<>{});
}

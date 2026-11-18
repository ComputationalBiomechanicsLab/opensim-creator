#include "forward_dynamics_solver.h"

#include <libopynsim/solvers/forward_dynamics_solver.h>
#include <libopynsim/solvers/integrator_settings.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state_stage.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>

namespace nb = nanobind;

void opyn::python::def_forward_dynamics_solver(nb::module_& m)
{
    nb::class_<ForwardDynamicsSolver> cls(
        m,
        "ForwardDynamicsSolver",
        R"(
            A solver that integrates the forward dynamics of a :class:`Model`
            + :class:`ModelState` pair.

            The solver manages a :class:`ModelState` that it integrates forward
            in time to a caller-specified timepoint (see :meth:`integrate_to`).
        )");
    cls.def(
        nb::init<Model, std::optional<ModelState>, std::optional<IntegratorSettings>>{},
        nb::arg("model"),
        nb::arg("model_state") = std::nullopt,
        nb::arg("integrator_settings") = std::nullopt,
        R"(
            Constructs a :class:`ForwardDynamicsSolver` for ``model``.

            Args:
                model (Model): The model that is being integrated.
                model_state (ModelState): If supplied, the state of ``model`` that the solver begins
                    integration from. Otherwise, defaults to ``model.initial_state()``.
                integrator_settings (IntegratorSettings): The integrator settings of the solvers's integrator.
        )");
    cls.def(
        "integrate_to",
        &ForwardDynamicsSolver::integrate_to,
        nb::arg("time"),
        nb::arg("realized_to") = ModelStateStage::report,
        R"(
            Forward-integrates the solvers's :class:`ModelState` to ``time``.

            Args:
                time (float): The endpoint that the integrator should integrate towards. Must be
                    greater than or equal to the solver's current time.
                realized_to (ModelStateStage): The stage at which the returned :class:`ModelState`
                    should be realized to by the solver.

            Returns:
                A copy of the solver's :class:`ModelState`, representing the model's state
                at ``time`` realized to ``realized_to``.
        )");
    cls.def_prop_ro(
        "model_state",
        &ForwardDynamicsSolver::model_state,
        R"(
            Returns a copy of the solver's :class:`ModelState`.
        )");
}

#include "solvers.h"

#include <libopynsim/solvers/forward_dynamics_solver.h>
#include <libopynsim/solvers/integrator_settings.h>
#include <libopynsim/solvers/model_warper.h>
#include <libopynsim/model.h>
#include <libopynsim/model_specification.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>

#include <optional>

namespace nb = nanobind;
using namespace opyn;

namespace
{
    void def_model_warper(nb::module_& m)
    {
        nb::class_<ModelWarper> cls(m, "ModelWarper", R"(
            A solver that can warp a source :class:`opynsim.ModelSpecification`.

            Instances of this class are usually constructed from data files
            (scaling documents) via :meth:`from_xml`.
        )");
        cls.def_static("from_xml", ModelWarper::from_xml, nb::arg("source"), R"(
            Returns a :class:`ModelWarper` parsed from an XML file (``source``).

            Args:
                source: Filesystem path to a ``<ModelWarperV3Document>`` XML file (e.g. from
                    OpenSim Creator's model warper).

            Returns:
                A :class:`ModelWarper`, initialized with the scaling steps and parameters
                defined in ``source``.

            Raises:
                RuntimeError: If ``source`` cannot be found, read, or is invalid.
        )");
        cls.def(nb::init<>{}, "Constructs a blank :class:`ModelWarper` that performs no warping operations (i.e. an identity warp).");
        cls.def("warp", &ModelWarper::warp, nb::arg("model_specification"), R"(
            Returns a warped copy of ``model_specification``.

            Args:
                model_specification: An :class:`opynsim.ModelSpecification` that will be copied and
                    warped by the model warper. Must be compatible with the model warping
                    pipeline that the model warper executes.

            Returns:
                An :class:`opynsim.ModelSpecification` with all warping (scaling) steps applied to it.
                For performance reasons, the warper may produce resources that are stored in-memory. Use
                :meth:`opynsim.ModelSpecification.flush_in_memory_resources_to` to flush those resources to
                disk, if you need to save those resources.

            Raises:
                RuntimeError: If ``model_specification`` cannot be warped by this model warper's
                    warping pipeline.
        )");
    }

    void def_integrator_settings(nb::module_& m)
    {
        nb::class_<IntegratorSettings> cls(m, "IntegratorSettings", R"(
            Settings for a forward integrator (e.g. as used by :class:`ForwardDynamicsSolver`).

            **Note**: Modifying integrator settings can have a large effect on its performance
            and behavior. When tweaking the settings, it is recommended to re-validate outcomes.
        )");
        cls.def(nb::init<>{});
    }

    void def_forward_dynamics_solver(nb::module_& m)
    {
        nb::class_<ForwardDynamicsSolver> cls(
            m,
            "ForwardDynamicsSolver",
            R"(
                A solver that integrates the forward dynamics of a :class:`Model`
                + :class:`ModelState` pair.

                The solver stores a :class:`ModelState` that it integrates forward
                in time to a caller-specified timepoint (see :meth:`integrate_to`).
            )"
        );
        cls.def(
            nb::init<Model, ModelState, std::optional<IntegratorSettings>>{},
            nb::arg("model"),
            nb::arg("model_state"),
            nb::arg("integrator_settings") = std::nullopt,
            R"(
                Constructs a :class:`ForwardDynamicsSolver` of ``model`` in ``model_state``.

                Args:
                    model (Model): The model that is being integrated.
                    model_state (ModelState): The state of ``model`` that the solver begins integration from.
                    integrator_settings (IntegratorSettings): The integrator settings of the solvers's integrator.
            )"
        );
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
                    A copy of the solver's :class:`ModelState` representing the model's state
                    at ``time`` realized to ``realized_to``.
            )"
        );
    }
}

void opyn::init_solvers_submodule(nanobind::module_& solvers_module)
{
    def_model_warper(solvers_module);
    def_integrator_settings(solvers_module);
    def_forward_dynamics_solver(solvers_module);
}

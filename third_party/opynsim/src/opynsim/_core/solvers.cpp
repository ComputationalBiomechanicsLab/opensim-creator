#include "solvers.h"

#include <libopynsim/solvers/model_warper.h>
#include <libopynsim/model_specification.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/filesystem.h>

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
}

void opyn::init_solvers_submodule(nanobind::module_& solvers_module)
{
    def_model_warper(solvers_module);
}

#include <_opynsim_native/tps3d.h>
#include <_opynsim_native/ui.h>

#include <liboscar/platform/log_level.h>
#include <liboscar/utilities/enum_helpers.h>
#include <libopynsim/model.h>
#include <libopynsim/model_specification.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <libopynsim/opynsim.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/filesystem.h>

using namespace opyn;

namespace nb = nanobind;

namespace
{
    osc::LogLevel to_oscar_log_level(int python_logging_level)
    {
        // These are dictated by the python documentation: https://docs.python.org/3/library/logging.html#logging-levels
        switch (python_logging_level) {
        case 10: return osc::LogLevel::debug;     // logging.DEBUG
        case 20: return osc::LogLevel::info;      // logging.INFO
        case 30: return osc::LogLevel::warn;      // logging.WARNING
        case 40: return osc::LogLevel::err;       // logging.ERROR
        case 50: return osc::LogLevel::critical;  // logging.CRITICAL
        case 0 : [[fallthrough]];                 // logging.NOTSET
        default: return osc::LogLevel::DEFAULT;
        }
    }
}

NB_MODULE(_opynsim_native, _opynsim_native_module)  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
{
    // Libraries should be quiet by default - unless there's an error
    opyn::set_log_level(osc::LogLevel::err);

    // Globally initialize the opynsim API (Simbody, OpenSim, oscar)
    opyn::init();

    // Initialize `tps3d` submodule.
    {
        auto tps3d_submodule = _opynsim_native_module.def_submodule("tps3d");
        init_tps3d_submodule(tps3d_submodule);
    }

    // Initialize `ui` submodule.
    {
        auto ui_submodule = _opynsim_native_module.def_submodule("ui");
        init_ui_submodule(ui_submodule);
    }

    // Initialize top-level functions/classes
    {
        nb::class_<ModelSpecification> model_specification_class(_opynsim_native_module, "ModelSpecification");
        model_specification_class.def(nb::init<>());  // Define default constructor

        nb::class_<Model> model_class(_opynsim_native_module, "Model");
        model_class.def("initial_state", &Model::initial_state);
        model_class.def("realize", &Model::realize);

        nb::class_<ModelState> model_state_class(_opynsim_native_module, "ModelState");

        static_assert(osc::num_options<ModelStateStage>() == 6);
        nb::enum_<ModelStateStage>(_opynsim_native_module, "ModelStateStage")
            .value("TIME",         ModelStateStage::time)
            .value("POSITION",     ModelStateStage::position)
            .value("VELOCITY",     ModelStateStage::velocity)
            .value("DYNAMICS",     ModelStateStage::dynamics)
            .value("ACCELERATION", ModelStateStage::acceleration)
            .value("REPORT",       ModelStateStage::report);

        _opynsim_native_module.def(
            "set_logging_level",
            [](int python_logging_level) { opyn::set_log_level(to_oscar_log_level(python_logging_level)); },
            nb::arg("python_logging_level"),
            R"(
                Set the global logging level for OPynSim.

                By default, OPynSim minimizes log output so that downstream Python
                applications can control their own logging configuration. However,
                some modelling components (particularly those originating from OpenSim)
                emit useful diagnostic information to the log.

                This function configures OPynSim's internal logging verbosity using
                standard Python logging levels.

                Args:
                    python_logging_level (int): A logging level from the Python (:mod:`logging`) module
                        such as ``logging.DEBUG``, ``logging.INFO``, ``logging.WARNING``, ``logging.ERROR``,
                        or ``logging.CRITICAL``.

            )"
        );

        _opynsim_native_module.def(
            "import_osim_file",
            [](const std::filesystem::path& osim_path) { return opyn::import_osim_file(osim_path); },
            nb::arg("osim_file_path"),
            R"(
                Returns a :class:`ModelSpecification` imported from an OpenSim (.osim) file on the
                caller's filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        _opynsim_native_module.def(
            "add_opensim_geometry_directory",
            [](const std::filesystem::path& geometry_directory) { opyn::add_opensim_geometry_directory(geometry_directory); },
            nb::arg("geometry_directory_path"),
            R"(
                Globally adds the provided path to OpenSim's geometry search path, which
                OpenSim uses whenever it cannot find a mesh file that an OpenSim model
                file references.

                By default, relative paths in OpenSim models are resolved as ``Geometry/${mesh_path}``
                in the directory of the model file (if it's on disk). For example, if you
                have an OpenSim model file ``directory/model.osim`` that contains a ``<Mesh>``
                component with a ``mesh_file`` called ``r_pelvis.vtp``, then OpenSim will search:

                - ``directory/Geometry/r_pelvis.vtp``
                - ``${geometry_directory_path[0]}/r_pelivs.vtp``
                - ``${geometry_directory_path[1]}/r_pelivs.vtp``

                Where ``${geometry_directory_path[i]}`` is the ``i``\th ``geometry_directory_path``
                provided to this function.

                Args:
                    geometry_directory_path: An absolute path to the fallback directory.
            )"
        );

        _opynsim_native_module.def(
            "compile_specification",
            opyn::compile_specification,
            nb::arg("model_specification"),
            R"(
                Compiles a :class:`ModelSpecification` into a :class:`Model`.

                The compilation process:

                - Validates the :class:`ModelSpecification`'s components (properties, subcomponents,
                  and sockets), throwing an exception if the specification is invalid in some way.
                - Assembles a physics system from the validated specification, throwing an exception
                  if the physics system cannot be assembled (e.g. if the contains impossible-to-satisfy
                  joints, or invalid muscle definitions).

                Raises:
                    RuntimeError: If the compilation process failed in some way. It is assumed that
                        the provided :class:`ModelSpecification` is valid.

            )"
        );
    }
}

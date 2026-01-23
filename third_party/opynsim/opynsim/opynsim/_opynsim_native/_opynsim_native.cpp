#include <_opynsim_native/tps3d.h>
#include <_opynsim_native/ui.h>

#include <liboscar/platform/log_level.h>
#include <liboscar/utils/enum_helpers.h>
#include <libopynsim/model.h>
#include <libopynsim/model_specification.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <libopynsim/opynsim.h>
#include <nanobind/nanobind.h>

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
        model_specification_class.def("compile", &ModelSpecification::compile);

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
            nb::arg("python_logging_level")
        );

        _opynsim_native_module.def(
            "import_osim_file",
            [](const nb::str& osim_path) { return opyn::import_osim_file(osim_path.c_str()); },
            nb::arg("osim_file_path")
        );

        _opynsim_native_module.def(
            "add_geometry_directory",
            [](const nb::str& geometry_directory) { opyn::add_geometry_directory(geometry_directory.c_str()); },
            nb::arg("geometry_directory_path")
        );
    }
}

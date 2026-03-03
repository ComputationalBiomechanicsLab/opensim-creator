#include "ui.h"

#include <libopynsim/ui/show_hello_ui.h>
#include <libopynsim/ui/show_model_in_state.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>
#include <nanobind/nanobind.h>

namespace nb = nanobind;

void opyn::init_ui_submodule(nanobind::module_& ui_module)
{
    ui_module.def(
        "show_hello_ui",
        show_hello_ui,
        R"(
            Displays OPynSim's 'hello world' user interface in a window.

            Blocks and runs the GUI main loop until the window is closed.
        )"
    );
    ui_module.def(
        "show_model_in_state",
        show_model_in_state,
        nb::arg("model"),
        nb::arg("state"),
        R"(
            Displays a visualizer for the given :class:`opynsim.Model` + :class:`opynsim.ModelState` in
            a window.

            Blocks and runs the GUI main loop until the window is closed.
        )"
    );
}

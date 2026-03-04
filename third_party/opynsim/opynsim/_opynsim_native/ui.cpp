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
        nb::kw_only{},
        nb::arg("zoom_to_fit") = true,
        R"(
            Displays an interactive visualizer for the given :class:`opynsim.Model` + :class:`opynsim.ModelState`
            in a window.

            Blocks and runs the GUI main loop until the window is closed.

            Args:
                model (opynsim.Model): The model to render.
                state (opynsim.ModelState): The state of the model to render. Should be realized to at least :attr:`opynsim.ModelStateStage.REPORT`.
                zoom_to_fit (bool): Tells the renderer to automatically set up the camera to focus on the center of the bounds of the scene at a distance that can see the entire scene.
        )"
    );
}

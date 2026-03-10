#include "ui.h"

#include <opynsim/_core/core.h>

#include <libopynsim/ui/show_hello_ui.h>
#include <libopynsim/ui/show_model_in_state.h>
#include <libopynsim/ui/ui_callbacks.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>
#include <nanobind/nanobind.h>

namespace nb = nanobind;
using namespace opyn;

namespace
{
    // Returns `UiCallbacks` that are suitable for making the UI play properly
    // in a Python interpreter.
    UiCallbacks default_python_callbacks()
    {
        return {
            // Every UI loop tick, the Python interpreter should be checked
            // for signals (e.g. keyboard interrupts) so that the user can
            // close the UI from the terminal with `Ctrl+C` (for example).
            .on_tick_begin = []
            {
                if (PyErr_CheckSignals() != 0) {
                    throw nb::python_error{};
                }
            }
        };
    }
}

void opyn::init_ui_submodule(nanobind::module_& ui_module)
{
    ui_module.def(
        "show_hello_ui",
        [] { show_hello_ui(get_lazy_loaded_opynsim_app(), default_python_callbacks()); },
        R"(
            Displays OPynSim's 'hello world' user interface in a window.

            Blocks and runs the GUI main loop until the window is closed.
        )"
    );
    ui_module.def(
        "show_model_in_state",
        [](const Model& model, const ModelState& model_state, bool zoom_to_fit)
        {
            show_model_in_state(
                get_lazy_loaded_opynsim_app(),
                model,
                model_state,
                zoom_to_fit,
                default_python_callbacks()
            );
        },
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

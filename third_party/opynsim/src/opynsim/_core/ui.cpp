#include "ui.h"

#include <opynsim/_core/core.h>

#include <libopynsim/ui/show_hello_ui.h>
#include <libopynsim/ui/show_model_in_state.h>
#include <libopynsim/ui/ui_callbacks.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/pair.h>

#include <array>

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
        [](
            const Model& model,
            const ModelState& model_state,
            std::pair<int, int> dimensions,
            std::array<float, 4> background_color,
            bool zoom_to_fit,
            bool draw_floor)
        {
            show_model_in_state(
                get_lazy_loaded_opynsim_app(),
                model,
                model_state,
                osc::Vector2{dimensions.first, dimensions.second},
                osc::Color{background_color[0], background_color[1], background_color[2], background_color[3]},
                zoom_to_fit,
                draw_floor,
                default_python_callbacks()
            );
        },
        nb::arg("model"),
        nb::arg("model_state"),
        nb::kw_only{},
        nb::arg("dimensions") = std::make_pair(640, 480),
        nb::arg("background_color") = std::to_array({1.0f, 1.0f, 1.0f, 1.0f}),
        nb::arg("zoom_to_fit") = true,
        nb::arg("draw_floor") = false,
        R"(
            Displays an interactive visualizer for the given :class:`opynsim.Model` + :class:`opynsim.ModelState`
            in a window.

            Blocks and runs the GUI main loop until the window is closed.

            Args:
                model (opynsim.Model): The model to show.
                model_state (opynsim.ModelState): The state of the model to show. Should be realized to at least :attr:`opynsim.ModelStateStage.REPORT`.
                dimensions (tuple[int, int]): The desired output resolution (width, height) of the window in device-independent pixels.
                background_color: The desired background color of the rendered scene, specified as normalized floats representing RGBA.
                zoom_to_fit (bool): Tells the ui to automatically set up the camera to focus on the center of the bounds of the scene at a distance that can see the entire scene.
                draw_floor (bool): Draws a chequered floor.
        )"
    );
}

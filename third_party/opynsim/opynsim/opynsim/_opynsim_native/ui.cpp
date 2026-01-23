#include "ui.h"

#include <libopynsim/ui/hello_ui.h>
#include <libopynsim/ui/model_viewer.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>
#include <nanobind/nanobind.h>

void opyn::init_ui_submodule(nanobind::module_& ui_module)
{
    ui_module.def("hello_ui", show_hello_ui);
    ui_module.def("view_model_in_state", view_model_in_state);
}

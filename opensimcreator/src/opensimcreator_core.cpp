#include <libopensimcreator/Platform/OpenSimCreatorApp.h>

#include <nanobind/nanobind.h>

using namespace osc;
namespace nb = nanobind;

NB_MODULE(opensimcreator_core, m) {
    nb::class_<OpenSimCreatorApp>(m, "App")
        .def(nb::init<>());
}

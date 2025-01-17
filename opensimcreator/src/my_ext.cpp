
#include <libOpenSimCreator/Platform/OpenSimCreatorApp.h>

#include <nanobind/nanobind.h>

using namespace osc;
namespace nb = nanobind;

NB_MODULE(_my_ext_impl, m) {
    nb::class_<OpenSimCreatorApp>(m, "App")
        .def(nb::init<>());
}

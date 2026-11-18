#include "color.h"

#include <liboscar/graphics/color.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include <format>
#include <functional>

using namespace opyn;
using osc::Color;
namespace nb = nanobind;

void opyn::python::def_color(nanobind::module_& m)
{
    nb::class_<Color> cls(m, "Color", R"(
        Represents an RGBA color in an sRGB color space with normalized 32-bit floating-point components.
    )");
    static_assert(Color{} == Color::clear());
    cls.def(nb::init<>{});
    cls.def(
        nb::init<float, float, float, float>{},
        nb::arg("r"),
        nb::arg("g"),
        nb::arg("b"),
        nb::arg("a") = 1.0f);
    cls.def_prop_ro_static("clear", [](nb::handle) { return Color::clear(); }, nb::rv_policy::copy);
    cls.def_prop_ro_static("white", [](nb::handle) { return Color::white(); }, nb::rv_policy::copy);
    cls.def_prop_ro_static("black", [](nb::handle) { return Color::black(); }, nb::rv_policy::copy);
    cls.def_prop_ro_static("red", [](nb::handle) { return Color::red(); }, nb::rv_policy::copy);
    cls.def_prop_ro_static("green", [](nb::handle) { return Color::green(); }, nb::rv_policy::copy);
    cls.def_prop_ro_static("blue", [](nb::handle) { return Color::blue(); }, nb::rv_policy::copy);
    cls.def("__eq__", std::equal_to<Color>{});
    cls.def("__hash__", [](const Color& c) { return std::hash<Color>{}(c); });
    cls.def("__repr__", [](const Color& c) { return std::format("{}", c); });
    cls.def_prop_rw(
        "r", [](const Color& c) { return c.r; }, [](Color& self, float rhs) { self.r = rhs; });
    cls.def_prop_rw(
        "g", [](const Color& c) { return c.g; }, [](Color& self, float rhs) { self.g = rhs; });
    cls.def_prop_rw(
        "b", [](const Color& c) { return c.b; }, [](Color& self, float rhs) { self.b = rhs; });
    cls.def_prop_rw(
        "a", [](const Color& c) { return c.a; }, [](Color& self, float rhs) { self.a = rhs; });
}

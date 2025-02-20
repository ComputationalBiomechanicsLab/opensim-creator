#include <libopensimcreator/Platform/OpenSimCreatorApp.h>

#include <liboscar/Utils/Assertions.h>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

using namespace osc;
namespace nb = nanobind;

using Vec3Array = nb::ndarray<const double, nb::shape<-1, 3>, nb::device::cpu>;

namespace
{
    void calc_tps_coefficients(
        [[maybe_unused]] const Vec3Array& source_landmarks,
        [[maybe_unused]] const Vec3Array& destination_landmarks)
    {
        OSC_ASSERT(source_landmarks.size() == destination_landmarks.size());
    }
}

NB_MODULE(_opensimcreator_native, m) {
    m.def(
        "solve_coefficients",
        calc_tps_coefficients,
        nb::arg("source_landmarks"),
        nb::arg("destination_landmarks"),
        "Pairs `source_landmarks` with `destination_landmarks` and uses the pairing to compute the Thin-Plate Spline (coefficients) of the pairing"
    );
}

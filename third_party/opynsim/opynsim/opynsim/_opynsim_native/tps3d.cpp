#include "tps3d.h"

#include <_opynsim_native/nanobind_x_mdspan.h>
#include <_opynsim_native/nanobind_x_simbody.h>

#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <libopynsim/shims/cpp23/mdspan.h>
#include <libopynsim/ui/hello_ui.h>
#include <libopynsim/utilities/assertions.h>
#include <libopynsim/utilities/tps3d.h>

#include <nanobind/ndarray.h>
#include <SimTKcommon/SmallMatrix.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>

namespace nb = nanobind;
using namespace opyn;

namespace
{
    std::unique_ptr<TPSCoefficients3D<double>> calc_tps_coefficients(
        const nb::ndarray<const double, nb::shape<-1, 3>, nb::device::cpu>& source_landmarks,
        const nb::ndarray<const double, nb::shape<-1, 3>, nb::device::cpu>& destination_landmarks)
    {
        OPYN_ASSERT_ALWAYS(source_landmarks.size() == destination_landmarks.size() && "there must be an equal amount of source/destination landmarks");
        OPYN_ASSERT_ALWAYS(source_landmarks.size() != 0 && "at least one pair of landmarks must be provided");

        const auto source_landmarks_mdspan = to_mdspan(source_landmarks);
        const auto destination_landmarks_mdspan = to_mdspan(destination_landmarks);

        // Solve the coefficients
        return std::make_unique<TPSCoefficients3D<double>>(TPSCalcCoefficients(source_landmarks_mdspan, destination_landmarks_mdspan));
    }

    nb::ndarray<double, nb::shape<3>, nb::device::cpu, nb::numpy> warp_point(
        const TPSCoefficients3D<double>& coefficients,
        const nb::ndarray<const double, nb::shape<3>, nb::device::cpu>& python_vec3d)
    {
        const SimTK::Vec3 input = to_vec(python_vec3d);
        const SimTK::Vec3 output = TPSWarpPoint(coefficients, input);
        return to_owned_numpy_array(output);
    }

    std::string repr(const TPSCoefficients3D<double>& coefs)
    {
        std::stringstream ss;
        ss << "<opynsim.tps3d.Coefficients ";
        ss << "a1 = " << coefs.a1 << ", ";
        ss << "a2 = " << coefs.a2 << ", ";
        ss << "a3 = " << coefs.a3 << ", ";
        ss << "a4 = " << coefs.a4 << ", ";
        ss << "non_affine_terms = [" << coefs.nonAffineTerms.size() << " values]>";
        return std::move(ss).str();
    }
}

void opyn::init_tps3d_submodule(nanobind::module_& tps3d_module)
{
    // class: TPSCoefficients3D
    nb::class_<TPSCoefficients3D<double>>(tps3d_module, "TPSCoefficients3D")
        .def("__repr__",
            [](const TPSCoefficients3D<double>& coefs) { return repr(coefs); }
        )
        .def_prop_ro("a1",
            [](const TPSCoefficients3D<double>& coefs) { return to_owned_numpy_array(coefs.a1); },
            nb::rv_policy::take_ownership
        )
        .def_prop_ro("a2",
            [](const TPSCoefficients3D<double>& coefs) { return to_owned_numpy_array(coefs.a2); },
            nb::rv_policy::take_ownership
        )
        .def_prop_ro("a3",
            [](const TPSCoefficients3D<double>& coefs) { return to_owned_numpy_array(coefs.a3); },
            nb::rv_policy::take_ownership
        )
        .def_prop_ro("a4",
            [](const TPSCoefficients3D<double>& coefs) { return to_owned_numpy_array(coefs.a4); },
            nb::rv_policy::take_ownership
        )
        .def("warp_point",
            warp_point,
            nb::arg("point"),
            "Warps a single 3D point"
        );

    tps3d_module.def(
        "solve_coefficients",
        calc_tps_coefficients,
        nb::arg("source_landmarks"),
        nb::arg("destination_landmarks"),
        "Pairs `source_landmarks` with `destination_landmarks` and uses the pairing to compute the Thin-Plate Spline (coefficients) of the pairing"
    );
}

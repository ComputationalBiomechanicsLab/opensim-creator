#include "tps3d.h"

#include <_opynsim_native/nanobind_x_mdspan.h>
#include <_opynsim_native/nanobind_x_simbody.h>

#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <libopynsim/shims/cpp23/mdspan.h>
#include <libopynsim/ui/show_hello_ui.h>
#include <libopynsim/tps3d.h>
#include <liboscar/utilities/assertions.h>

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
        OSC_ASSERT_ALWAYS(source_landmarks.size() == destination_landmarks.size() && "there must be an equal amount of source/destination landmarks");
        OSC_ASSERT_ALWAYS(source_landmarks.size() != 0 && "at least one pair of landmarks must be provided");

        const auto source_landmarks_mdspan = to_mdspan(source_landmarks);
        const auto destination_landmarks_mdspan = to_mdspan(destination_landmarks);

        // Solve the coefficients
        return std::make_unique<TPSCoefficients3D<double>>(tps3d_solve_coefficients(source_landmarks_mdspan, destination_landmarks_mdspan));
    }

    nb::ndarray<double, nb::shape<3>, nb::device::cpu, nb::numpy> warp_point(
        const TPSCoefficients3D<double>& coefficients,
        const nb::ndarray<const double, nb::shape<3>, nb::device::cpu>& python_vec3d)
    {
        const SimTK::Vec3 input = to_vec(python_vec3d);
        const SimTK::Vec3 output = tps3d_warp_point(coefficients, input);
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
        ss << "non_affine_terms = [" << coefs.non_affine_terms.size() << " values]>";
        return std::move(ss).str();
    }
}

void opyn::init_tps3d_submodule(nanobind::module_& tps3d_module)
{
    // class: TPSCoefficients3D
    nb::class_<TPSCoefficients3D<double>>(tps3d_module, "TPSCoefficients3D", "Represents Thin-Plate Spline (TPS) coefficients in 3D that were calculated from corresponding pairs of points.")
        .def("__repr__",
            [](const TPSCoefficients3D<double>& coefs) { return repr(coefs); }
        )
        .def_prop_ro("a1",
            [](const TPSCoefficients3D<double>& coefs) { return to_owned_numpy_array(coefs.a1); },
            nb::rv_policy::take_ownership,
            R"(
                The first term of the warping equation. Some systems treat this as the translation component of the warp.
            )"
        )
        .def_prop_ro("a2",
            [](const TPSCoefficients3D<double>& coefs) { return to_owned_numpy_array(coefs.a2); },
            nb::rv_policy::take_ownership,
            R"(
                The second term of the warping equation. Some systems treat this as the first column of a 3x3 scale + rotation matrix.
            )"
        )
        .def_prop_ro("a3",
            [](const TPSCoefficients3D<double>& coefs) { return to_owned_numpy_array(coefs.a3); },
            nb::rv_policy::take_ownership,
            R"(
                The second term of the warping equation. Some systems treat this as the second column of a 3x3 scale + rotation matrix.
            )"
        )
        .def_prop_ro("a4",
            [](const TPSCoefficients3D<double>& coefs) { return to_owned_numpy_array(coefs.a4); },
            nb::rv_policy::take_ownership,
            R"(
                The second term of the warping equation. Some systems treat this as the third column of a 3x3 scale + rotation matrix.
            )"
        )
        .def("warp_point",
            warp_point,
            nb::arg("point"),
            R"(
                Warps a single 3D point by putting ``point`` through the warping equation.

                Args:
                    point: a 3-element ndarray.

                Returns:
                    A 3-element ndarray that represents the warped point.
            )"
        );

    tps3d_module.def(
        "solve_coefficients",
        calc_tps_coefficients,
        nb::arg("source_landmarks"),
        nb::arg("destination_landmarks"),
        R"(
            Returns Thin-Plate Spline (TPS) warping coefficients solved by pairing N
            "source" points (source_landmarks) with N "destination" points (destination_landmarks).

            Args:
                source_landmarks: An (N, 3) array of 3D points.
                destination_landmarks: An (N, 3) array of 3D points.

            Returns:
                A TPSCoefficients3D object that contains the calculated coefficients.
        )"
    );
}

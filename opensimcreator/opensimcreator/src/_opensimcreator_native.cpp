#include <liboscar/Utils/Assertions.h>
#include <libopensimcreator/Utils/TPS3D.h>
#include <libopensimcreator/Platform/OpenSimCreatorApp.h>

#include <nanobind/stl/array.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

using namespace osc;
namespace nb = nanobind;

using SharedPythonVec3dArray = nb::ndarray<const double, nb::shape<-1, 3>, nb::device::cpu>;
using SharedPythonVec3d = nb::ndarray<const double, nb::shape<3>, nb::device::cpu>;

namespace
{
    const TPSCoefficients3D<float>* calc_tps_coefficients(
        SharedPythonVec3dArray source_landmarks,
        SharedPythonVec3dArray destination_landmarks)
    {
        OSC_ASSERT_ALWAYS(source_landmarks.size() == destination_landmarks.size() && "there must be an equal amount of source/destination landmarks");
        OSC_ASSERT_ALWAYS(source_landmarks.size() != 0 && "at least one pair of landmarks must be provided");

        // Pack the user-provided ndarrays into the solver's input structure.
        TPSCoefficientSolverInputs3D<float> inputs;
        inputs.landmarks.reserve(source_landmarks.size());
        for (size_t i = 0; i < source_landmarks.size(); ++i) {
            LandmarkPair3D<float>& p = inputs.landmarks.emplace_back();
            p.source[0] = static_cast<float>(source_landmarks(i, 0));
            p.source[1] = static_cast<float>(source_landmarks(i, 1));
            p.source[2] = static_cast<float>(source_landmarks(i, 2));
            p.destination[0] = static_cast<float>(source_landmarks(i, 0));
            p.destination[1] = static_cast<float>(source_landmarks(i, 2));
            p.destination[2] = static_cast<float>(source_landmarks(i, 3));
        }

        // Solve the coefficients
        return new TPSCoefficients3D{CalcCoefficients(inputs)};
    }

    nb::ndarray<double, nb::shape<3>, nb::device::cpu, nb::numpy> warp_point(const TPSCoefficients3D<float>& coefficients, SharedPythonVec3d python_vec3d)
    {
        const Vec3 input = {python_vec3d(0), python_vec3d(1), python_vec3d(2)};
        const Vec3 output = EvaluateTPSEquation(coefficients, input);
        double* dptr = new double[3];
        dptr[0] = output.x;
        dptr[1] = output.y;
        dptr[2] = output.z;
        nb::capsule owner{dptr, [](void* p) noexcept { delete[](double*) p; }};
        return {dptr, {}, owner};
    }

    //nb::ndarray<double, nb::shape<-1, 3>, nb::device::cpu, nb::numpy>
    void warp_points(
        const TPSCoefficients3D<float>& coefficients,
        SharedPythonVec3dArray python_vec3ds)
    {
        auto points = std::make_unique<Vec3[]>(python_vec3ds.size());
        for (size_t i = 0; i < python_vec3ds.size(); ++i) {
            points[i] = {
                static_cast<float>(python_vec3ds(i, 0)),
                static_cast<float>(python_vec3ds(i, 1)),
                static_cast<float>(python_vec3ds(i, 2)),
            };
        }
        ApplyThinPlateWarpToPointsInPlace(coefficients, {points.get(), python_vec3ds.size()}, 1.0f);

        // TODO: make them `double`s?
    }
}

NB_MODULE(_opensimcreator_native, m) {
    nb::class_<TPSCoefficients3D<float>>(m, "TPSCoefficients3D")
        .def("__repr__", [](const TPSCoefficients3D<float>&) { return "<opensimcreator.tps3d.Coefficients>"; })
        .def("warp_point", warp_point, nb::arg("point"), "Warps a single 3D point")
        .def("warp_points", warp_points, nb::arg("points"), "Warps a sequence of 3D points");

    m.def(
        "solve_coefficients",
        calc_tps_coefficients,
        nb::arg("source_landmarks"),
        nb::arg("destination_landmarks"),
        "Pairs `source_landmarks` with `destination_landmarks` and uses the pairing to compute the Thin-Plate Spline (coefficients) of the pairing"
    );
}

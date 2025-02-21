#include <liboscar/Utils/Assertions.h>
#include <libopensimcreator/Utils/TPS3D.h>
#include <libopensimcreator/Platform/OpenSimCreatorApp.h>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

using namespace osc;
namespace nb = nanobind;

using PythonVec3dArray = nb::ndarray<const double, nb::shape<-1, 3>, nb::device::cpu>;

namespace
{
    void calc_tps_coefficients(
        [[maybe_unused]] const PythonVec3dArray& source_landmarks,
        [[maybe_unused]] const PythonVec3dArray& destination_landmarks)
    {
        OSC_ASSERT_ALWAYS(source_landmarks.size() == destination_landmarks.size() && "there must be an equal amount of source/destination landmarks");
        OSC_ASSERT_ALWAYS(source_landmarks.size() != 0 && "at least one pair of landmarks must be provided");

        // Pack the user-provided ndarrays into the solver's input structure.
        TPSCoefficientSolverInputs3D inputs;
        inputs.landmarks.reserve(source_landmarks.size());
        for (size_t i = 0; i < source_landmarks.size(); ++i) {
            LandmarkPair3D& p = inputs.landmarks.emplace_back();
            p.source[0] = static_cast<float>(source_landmarks(i, 0));
            p.source[1] = static_cast<float>(source_landmarks(i, 0));
            p.source[2] = static_cast<float>(source_landmarks(i, 0));
            p.destination[0] = static_cast<float>(source_landmarks(i, 0));
            p.destination[1] = static_cast<float>(source_landmarks(i, 0));
            p.destination[2] = static_cast<float>(source_landmarks(i, 0));
        }

        // Solve the coefficients
        [[maybe_unused]] const TPSCoefficients3D coefficients = CalcCoefficients(inputs);

        // TODO: Unpack the coefficients into a python-friendly format.
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

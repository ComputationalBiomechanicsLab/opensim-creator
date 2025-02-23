#include <liboscar/Utils/Assertions.h>
#include <liboscar/Shims/Cpp23/mdspan.h>
#include <libopensimcreator/Utils/TPS3D.h>
#include <libopensimcreator/Platform/OpenSimCreatorApp.h>

#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

#include <cstddef>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>

using namespace osc;
namespace nb = nanobind;

namespace
{
    // Vec<N, T> --> `numpy.array` converter.
    //
    // Returns a numpy array that exposes a C++ `Vec<N, T>`'s data.
    template<size_t N, typename T>
    nb::ndarray<T, nb::shape<N>, nb::device::cpu, nb::numpy> to_ndarray(const Vec<N, T>& vec)
    {
        // Heap-allocate the data so that python can reference-count it
        auto* dptr = new T[N];
        std::uninitialized_copy_n(vec.data(), N, dptr);
        nb::capsule owner{dptr, [](void* p) noexcept { delete[] static_cast<T*>(p); }};
        return {dptr, {}, owner};
    }

    // Python `ndarray` --> Vec<N, T> converter
    //
    // Packs the given `ndarray` into a C++ `Vec<N, T>`
    template<size_t N, typename T>
    Vec<N, T> to_vec(const nb::ndarray<const T, nb::shape<N>, nb::device::cpu>& ndarray)
    {
        Vec<N, T> rv;
        for (size_t i = 0; i < N; ++i) {
            rv[i] = ndarray(i);
        }
        return rv;
    }
}

/*
namespace
{
    namespace detail
    {
        // Converts an `nb::ssize_t` extent from `nb::shape` to the equivalent
        // `std::extent`, as used by `std::mdspan`.
        template<nb::ssize_t NbExtent>
        constexpr size_t to_stdlib_extent() { return static_cast<size_t>(NbExtent); }
        template<>
        constexpr size_t to_stdlib_extent<nb::ssize_t{-1}>() { return std::dynamic_extent; }

        // Converts an `nb::shape` into the equilvanet `std::extents`, as used by `std::mdspan`.
        template<nb::ssize_t... NbExtents>
        constexpr auto to_stdlib_extents(nb::shape<NbExtents...>) -> cpp23::extents<size_t, to_stdlib_extent<NbExtents>()...>
        {
            return {};
        }

        template<typename NbShape>
        using ToStdlibExtents = decltype(to_stdlib_extents(std::declval<NbShape>()));
    }

    template<typename T, typename Shape>
    constexpr auto to_stdlib_mdspan(nb::ndarray<T, Shape, nb::device::cpu> ndary)
        -> cpp23::mdspan<T, detail::ToStdlibExtents<Shape>, cpp23::layout_stride>
    {
        // TODO: must
        const cpp23::extents<size_t, cpp23::dynamic_extent, 3> shape{inputs.landmarks.size()};
        const std::array<size_t, 2> strides = {6, 1};
        const cpp23::layout_stride::mapping mapping{shape, strides};
    }
}
*/

namespace
{
    const TPSCoefficients3D<double>* calc_tps_coefficients(
        nb::ndarray<const double, nb::shape<-1, 3>, nb::device::cpu> source_landmarks,
        nb::ndarray<const double, nb::shape<-1, 3>, nb::device::cpu> destination_landmarks)
    {
        OSC_ASSERT_ALWAYS(source_landmarks.size() == destination_landmarks.size() && "there must be an equal amount of source/destination landmarks");
        OSC_ASSERT_ALWAYS(source_landmarks.size() != 0 && "at least one pair of landmarks must be provided");

        // Pack the user-provided ndarrays into the solver's input structure.
        TPSCoefficientSolverInputs3D<double> inputs;
        inputs.landmarks.reserve(source_landmarks.shape(0));
        for (size_t i = 0; i < source_landmarks.shape(0); ++i) {
            LandmarkPair3D<double>& p = inputs.landmarks.emplace_back();
            p.source[0] = source_landmarks(i, 0);
            p.source[1] = source_landmarks(i, 1);
            p.source[2] = source_landmarks(i, 2);
            p.destination[0] = destination_landmarks(i, 0);
            p.destination[1] = destination_landmarks(i, 1);
            p.destination[2] = destination_landmarks(i, 2);
        }

        // Solve the coefficients
        return new TPSCoefficients3D<double>{TPSCalcCoefficients(inputs)};
    }

    nb::ndarray<double, nb::shape<3>, nb::device::cpu, nb::numpy> warp_point(
        const TPSCoefficients3D<double>& coefficients,
        nb::ndarray<const double, nb::shape<3>, nb::device::cpu> python_vec3d)
    {
        const Vec3d input = to_vec(python_vec3d);
        const Vec3d output = TPSWarpPoint(coefficients, input);

        return to_ndarray(output);
    }

    std::string repr(const TPSCoefficients3D<double>& coefs)
    {
        std::stringstream ss;
        ss << "<opensimcreator.tps3d.Coefficients ";
        ss << "a1 = " << coefs.a1 << ", ";
        ss << "a2 = " << coefs.a2 << ", ";
        ss << "a3 = " << coefs.a3 << ", ";
        ss << "a4 = " << coefs.a4 << ", ";
        ss << "non_affine_terms = [" << coefs.nonAffineTerms.size() << " values]>";
        return std::move(ss).str();
    }

    /*
    //nb::ndarray<double, nb::shape<-1, 3>, nb::device::cpu, nb::numpy>
    void warp_points(
        const TPSCoefficients3D<double>& coefficients,
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
        //TPSWarpPointsInPlace(coefficients, {points.get(), python_vec3ds.size()}, 1.0f);

        // TODO: make them `double`s?
    }
    */
}

NB_MODULE(_opensimcreator_native, m) {
    // class: TPSCoefficients3D
    nb::class_<TPSCoefficients3D<double>>(m, "TPSCoefficients3D")
        .def("__repr__", [](const TPSCoefficients3D<double>& coefs) { return repr(coefs); })
        .def_prop_ro("a1",
            [](const TPSCoefficients3D<double>& coefs) { return to_ndarray(coefs.a1); },
            nb::rv_policy::take_ownership
        )
        .def_prop_ro("a2",
            [](const TPSCoefficients3D<double>& coefs) { return to_ndarray(coefs.a2); },
            nb::rv_policy::take_ownership
        )
        .def_prop_ro("a3",
            [](const TPSCoefficients3D<double>& coefs) { return to_ndarray(coefs.a3); },
            nb::rv_policy::take_ownership
        )
        .def_prop_ro("a4",
            [](const TPSCoefficients3D<double>& coefs) { return to_ndarray(coefs.a4); },
            nb::rv_policy::take_ownership
        )
        .def("warp_point", warp_point, nb::arg("point"), "Warps a single 3D point");

    m.def(
        "solve_coefficients",
        calc_tps_coefficients,
        nb::arg("source_landmarks"),
        nb::arg("destination_landmarks"),
        "Pairs `source_landmarks` with `destination_landmarks` and uses the pairing to compute the Thin-Plate Spline (coefficients) of the pairing"
    );
}

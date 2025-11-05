#include <libopensimcreator/Shims/Cpp23/mdspan.h>
#include <libopensimcreator/Utils/TPS3D.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Maths/Vector.h>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>

using namespace osc;
namespace nb = nanobind;

namespace
{
    // Returns a caller-owned 1D numpy ndarray constructed from the elements of `vec`.
    template<typename T, size_t N>
    requires std::is_trivially_constructible_v<T>
    nb::ndarray<T, nb::shape<static_cast<nb::ssize_t>(N)>, nb::device::cpu, nb::numpy> to_owned_numpy_array(const Vector<T, N>& vec)
    {
        auto data = std::make_unique<T[]>(N);  // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
        std::uninitialized_copy_n(vec.data(), N, data.get());
        auto* handle = data.get();
        nb::capsule owner{data.release(), [](void* p) noexcept
        {
            delete[] static_cast<T*>(p);  // NOLINT(cppcoreguidelines-owning-memory)
        }};
        return {handle, {}, owner};
    }

    // Internal implementation details for `to_vec`.
    namespace detail
    {
        // Returns a `Vec` constructed from elements in `ndarray` at the specified indices.
        template<nb::ssize_t N, std::copy_constructible T, size_t... I>
        requires (sizeof...(I) == N)
        Vector<T, static_cast<size_t>(N)> to_vec(
            const nb::ndarray<const T, nb::shape<N>, nb::device::cpu>& ndarray,
            std::index_sequence<I...>)
        {
            return {ndarray(I)...};
        }
    }

    // Returns a `Vec` constructed from the given (1D) ndarray.
    template<std::copy_constructible T, nb::ssize_t N>
    Vector<T, static_cast<size_t>(N)> to_vec(const nb::ndarray<const T, nb::shape<N>, nb::device::cpu>& ndarray)
    {
        return detail::to_vec(ndarray, std::make_index_sequence<N>{});
    }

    // Internal implementation details for `to_mdspan`.
    namespace detail
    {
        // Converts a `nb::ssize_t`, as used by `nb::shape`, into the equivalent  `std::extent`, as
        // used by `std::mdspan`.
        template<nb::ssize_t NbExtent>
        constexpr size_t to_stdlib_extent() { return static_cast<size_t>(NbExtent); }
        template<>
        constexpr size_t to_stdlib_extent<nb::ssize_t{-1}>() { return std::dynamic_extent; }

        // Converts an `nb::shape`, as used by `nb::ndarray` into the equivalent `std::extents`, as
        // used by `std::mdspan`.
        template<nb::ssize_t... NbExtents, size_t... I>
        requires (sizeof...(NbExtents) == sizeof...(I))
        constexpr auto to_stdlib_extents(
            nb::shape<NbExtents...>,
            const int64_t* runtime_extents,
            std::index_sequence<I...>) -> cpp23::extents<size_t, to_stdlib_extent<NbExtents>()...>
        {
            return cpp23::extents<size_t, to_stdlib_extent<NbExtents>()...>{runtime_extents[I]...};
        }

        template<nb::ssize_t... NbExtents>
        constexpr auto to_stdlib_extents(nb::shape<NbExtents...> shape, const int64_t* extents) -> cpp23::extents<size_t, to_stdlib_extent<NbExtents>()...>
        {
            return to_stdlib_extents(shape, extents, std::make_index_sequence<sizeof...(NbExtents)>{});
        }

        // A type that's the equivalent `std::extents` of the given `nb::shape`.
        template<typename NbShape>
        using ToStdlibExtents = decltype(to_stdlib_extents(std::declval<NbShape>(), std::declval<const int64_t*>()));

        // Returns an array of strides that's compatible with `std::layout_stride::mapping`.
        template<typename T, nb::ssize_t... NbExtents>
        std::array<size_t, sizeof...(NbExtents)> to_stdlib_strides(const nb::ndarray<T, nb::shape<NbExtents...>, nb::device::cpu>& ndary)
        {
            std::array<size_t, sizeof...(NbExtents)> rv{};
            for (size_t i = 0; i < sizeof...(NbExtents); ++i) {
                rv[i] = static_cast<size_t>(ndary.stride(i));
            }
            return rv;
        }
    }

    // Returns a non-owning `std::mdspan` view of the given `nb::ndarray`.
    template<typename T, typename Shape>
    constexpr auto to_mdspan(nb::ndarray<T, Shape, nb::device::cpu> ndary)
        -> cpp23::mdspan<T, detail::ToStdlibExtents<Shape>, cpp23::layout_stride>
    {
        return {
            ndary.data(),
            cpp23::layout_stride::mapping{detail::to_stdlib_extents(Shape{}, ndary.shape_ptr()), detail::to_stdlib_strides(ndary)}
        };
    }
}

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
        return std::make_unique<TPSCoefficients3D<double>>(TPSCalcCoefficients(source_landmarks_mdspan, destination_landmarks_mdspan));
    }

    nb::ndarray<double, nb::shape<3>, nb::device::cpu, nb::numpy> warp_point(
        const TPSCoefficients3D<double>& coefficients,
        const nb::ndarray<const double, nb::shape<3>, nb::device::cpu>& python_vec3d)
    {
        const Vector3d input = to_vec(python_vec3d);
        const Vector3d output = TPSWarpPoint(coefficients, input);
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

NB_MODULE(_opynsim_native, m) {  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
    // class: TPSCoefficients3D
    nb::class_<TPSCoefficients3D<double>>(m, "TPSCoefficients3D")
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

    m.def(
        "solve_coefficients",
        calc_tps_coefficients,
        nb::arg("source_landmarks"),
        nb::arg("destination_landmarks"),
        "Pairs `source_landmarks` with `destination_landmarks` and uses the pairing to compute the Thin-Plate Spline (coefficients) of the pairing"
    );
}


#pragma once

#include <nanobind/ndarray.h>
#include <SimTKcommon/SmallMatrix.h>

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace opyn
{
    // Returns a caller-owned 1D numpy ndarray constructed from the elements of `vec`.
    template<typename T, int N>
    requires std::is_trivially_constructible_v<T>
    nanobind::ndarray<T, nanobind::shape<static_cast<nanobind::ssize_t>(N)>, nanobind::device::cpu, nanobind::numpy> to_owned_numpy_array(
        const SimTK::Vec<N, T>& vec)
    {
        auto data = std::make_unique<T[]>(N);  // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
        std::uninitialized_copy_n(&vec[0], N, data.get());
        auto* handle = data.get();
        nanobind::capsule owner{data.release(), [](void* p) noexcept
        {
            delete[] static_cast<T*>(p);  // NOLINT(cppcoreguidelines-owning-memory)
        }};
        return {handle, {}, owner};
    }

    // Internal implementation details for `to_vec`.
    namespace detail
    {
        // Returns a `Vec` constructed from elements in `ndarray` at the specified indices.
        template<nanobind::ssize_t N, std::copy_constructible T, size_t... I>
        requires (sizeof...(I) == N)
        SimTK::Vec<static_cast<int>(N), T> to_vec(
            const nanobind::ndarray<const T, nanobind::shape<N>, nanobind::device::cpu>& ndarray,
            std::index_sequence<I...>)
        {
            return {ndarray(I)...};
        }
    }

    // Returns a `Vec` constructed from the given (1D) ndarray.
    template<std::copy_constructible T, nanobind::ssize_t N>
    SimTK::Vec<static_cast<int>(N), T> to_vec(const nanobind::ndarray<const T, nanobind::shape<N>, nanobind::device::cpu>& ndarray)
    {
        return detail::to_vec(ndarray, std::make_index_sequence<N>{});
    }
}

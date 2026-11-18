#pragma once

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/color32.h>
#include <liboscar/maths/common_functions.h>
#include <liboscar/maths/functors.h>
#include <liboscar/maths/matrix3x3.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/triangle.h>
#include <liboscar/maths/vector.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <random>
#include <ranges>
#include <type_traits>
#include <vector>

namespace osc::tests
{
    template<typename T>
    T generate();

    // clang-format off
    template<> float generate();
    template<> int generate();
    template<> bool generate();
    template<> uint8_t generate();
    template<> Color generate();
    template<> Color32 generate();
    template<> Vector2 generate();
    template<> Vector3 generate();
    template<> Vector4 generate();
    template<> Matrix3x3 generate();
    template<> Matrix4x4 generate();
    template<> Triangle generate();
    // clang-format on

    std::vector<Vector3> generate_triangle_vertices();
    std::vector<Vector3> generate_vertices(size_t);
    std::vector<Vector3> generate_normals(size_t);
    std::vector<Vector2> generate_texture_coordinates(size_t);
    std::vector<Color> generate_colors(size_t);
    std::vector<Vector4> generate_tangent_vectors(size_t);

    std::vector<uint16_t> iota_index_range(size_t start, size_t end);

    template<std::ranges::range Range, class UnaryOperation>
    requires std::invocable<UnaryOperation, const typename Range::value_type&>
    auto project_into_vector(const Range& src, UnaryOperation op)
    {
        using std::begin;
        using std::end;

        std::vector<
            std::remove_cvref_t<decltype(std::invoke(op, std::declval<decltype(*begin(src))>()))>>
            rv;
        rv.reserve(std::distance(begin(src), end(src)));
        for (const auto& el : src) {
            rv.push_back(std::invoke(op, el));
        }
        return rv;
    }

    template<class T>
    std::vector<T> resized_vector_copy(
        const std::vector<T>& v, size_t new_size, const T& filler = {})
    {
        std::vector<T> rv;
        rv.reserve(new_size);
        if (v.size() < new_size) {
            rv.insert(rv.end(), v.begin(), v.end());
            rv.resize(new_size, filler);
        } else {
            rv.insert(rv.end(), v.begin(), v.begin() + new_size);
        }
        return rv;
    }
}

// Define "vector nearness" test assertions
#define OSC_VECTOR_NEAR_INTERNAL(val1, val2, tol, failure_macro)                                   \
    do {                                                                                           \
        auto&& _v1 = (val1);                                                                       \
        auto&& _v2 = (val2);                                                                       \
        auto&& _tol = (tol);                                                                       \
        if (not all_of(equal_within_absdiff(_v1, _v2, _tol))) {                                    \
            failure_macro << "Difference between " << #val1 << " and " << #val2                    \
                          << " exceeds tolerance " << _tol << '\n'                                 \
                          << std::format("    Actual:   {}\n", _v1)                                \
                          << std::format("    Expected: {}\n", _v2)                                \
                          << std::format("    Delta:    {}\n", abs(_v1 - _v2));                    \
        }                                                                                          \
    } while (0)
#define OSC_ASSERT_VECTOR_NEAR(val1, val2, tol) OSC_VECTOR_NEAR_INTERNAL(val1, val2, tol, FAIL())
#define OSC_EXPECT_VECTOR_NEAR(val1, val2, tol)                                                    \
    OSC_VECTOR_NEAR_INTERNAL(val1, val2, tol, ADD_FAILURE())

// Define "angle nearness" test assertions
#define OSC_ANGLE_NEAR_INTERNAL(val1, val2, tol, failure_macro)                                    \
    do {                                                                                           \
        using CA = std::common_type_t<decltype(val1), decltype(val2), decltype(tol)>;              \
        const CA _v1(val1);                                                                        \
        const CA _v2(val2);                                                                        \
        const CA _tol(tol);                                                                        \
        if (not equal_within_absdiff(_v1, _v2, _tol)) {                                            \
            failure_macro << std::format(                                                          \
                "Difference between " #val1 " ({}) and " #val2 " ({}) exceeds " #tol " ({})\n",    \
                _v1,                                                                               \
                _v2,                                                                               \
                _tol);                                                                             \
        }                                                                                          \
    } while (0)
#define OSC_ASSERT_ANGLE_NEAR(val1, val2, tol) OSC_ANGLE_NEAR_INTERNAL(val1, val2, tol, FAIL())
#define OSC_EXPECT_ANGLE_NEAR(val1, val2, tol)                                                     \
    OSC_ANGLE_NEAR_INTERNAL(val1, val2, tol, ADD_FAILURE())

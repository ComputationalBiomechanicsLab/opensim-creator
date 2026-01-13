#pragma once

#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/Color32.h>
#include <liboscar/maths/Matrix3x3.h>
#include <liboscar/maths/Matrix4x4.h>
#include <liboscar/maths/Transform.h>
#include <liboscar/maths/Triangle.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/maths/Vector4.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <random>
#include <ranges>
#include <vector>

namespace osc::tests
{
    template<typename T>
    T generate();

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

        std::vector<std::remove_cvref_t<decltype(std::invoke(op, std::declval<decltype(*begin(src))>()))>> rv;
        rv.reserve(std::distance(begin(src), end(src)));
        for (const auto& el : src) {
            rv.push_back(std::invoke(op, el));
        }
        return rv;
    }

    template<class T>
    std::vector<T> resized_vector_copy(const std::vector<T>& v, size_t new_size, const T& filler = {})
    {
        std::vector<T> rv;
        rv.reserve(new_size);
        if (v.size() < new_size) {
            rv.insert(rv.end(), v.begin(), v.end());
            rv.resize(new_size, filler);
        }
        else {
            rv.insert(rv.end(), v.begin(), v.begin() + new_size);
        }
        return rv;
    }
}

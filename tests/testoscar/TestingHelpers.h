#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Color32.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <random>
#include <ranges>
#include <vector>

namespace osc::testing
{
    std::default_random_engine& get_process_random_engine();

    template<typename T>
    T generate();

    template<> float generate();
    template<> int generate();
    template<> bool generate();
    template<> uint8_t generate();
    template<> Color generate();
    template<> Color32 generate();
    template<> Vec2 generate();
    template<> Vec3 generate();
    template<> Vec4 generate();
    template<> Mat3 generate();
    template<> Mat4 generate();
    template<> Triangle generate();

    std::vector<Vec3> generate_triangle_vertices();
    std::vector<Vec3> generate_vertices(size_t);
    std::vector<Vec3> generate_normals(size_t);
    std::vector<Vec2> generate_texture_coordinates(size_t);
    std::vector<Color> generate_colors(size_t);
    std::vector<Vec4> generate_tangent_vectors(size_t);

    std::vector<uint16_t> iota_index_range(size_t start, size_t end);

    template<std::ranges::range Range, class UnaryOperation>
    requires std::invocable<UnaryOperation, const typename Range::value_type&>
    auto project_into_vector(const Range& src, UnaryOperation op)
    {
        using std::begin;
        using std::end;

        std::vector<std::remove_const_t<std::remove_reference_t<decltype(op(std::declval<decltype(*begin(src))>()))>>> rv;
        rv.reserve(std::distance(begin(src), end(src)));
        for (const auto& el : src) {
            rv.push_back(op(el));
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


#include "test_helpers.h"

#include <liboscar/maths/math_helpers.h>

#include <algorithm>
#include <numeric>
#include <random>

using namespace osc;

namespace
{
    template<class Generator>
    auto generate_into_vector(size_t n, Generator f)
    {
        std::vector<decltype(f())> rv;
        rv.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            rv.push_back(f());
        }
        return rv;
    }

    std::default_random_engine& get_process_random_engine()
    {
        // the RNG is deliberately deterministic, so that
        // test errors are reproducible
        static std::default_random_engine e{};  // NOLINT(cert-msc32-c,cert-msc51-cpp)
        return e;
    }
}

template<> float osc::tests::generate()
{
    return static_cast<float>(std::uniform_real_distribution{}(get_process_random_engine()));
}

template<> int osc::tests::generate()
{
    return std::uniform_int_distribution{}(get_process_random_engine());
}

template<> bool osc::tests::generate()
{
    return generate<int>() % 2 == 0;
}

template<> uint8_t osc::tests::generate()
{
    return static_cast<uint8_t>(std::uniform_int_distribution{0, 255}(get_process_random_engine()));
}

template<> Color osc::tests::generate()
{
    return Color{generate<float>(), generate<float>(), generate<float>(), generate<float>()};
}

template<> Color32 osc::tests::generate()
{
    return {generate<uint8_t>(), generate<uint8_t>(), generate<uint8_t>(), generate<uint8_t>()};
}

template<> Vector2 osc::tests::generate()
{
    return Vector2{generate<float>(), generate<float>()};
}

template<> Vector3 osc::tests::generate()
{
    return Vector3{generate<float>(), generate<float>(), generate<float>()};
}

template<> Vector4 osc::tests::generate()
{
    return Vector4{generate<float>(), generate<float>(), generate<float>(), generate<float>()};
}

template<> Matrix3x3 osc::tests::generate()
{
    return Matrix3x3{generate<Vector3>(), generate<Vector3>(), generate<Vector3>()};
}

template<> Matrix4x4 osc::tests::generate()
{
    return Matrix4x4{generate<Vector4>(), generate<Vector4>(), generate<Vector4>(), generate<Vector4>()};
}

template<> Triangle osc::tests::generate()
{
    return Triangle{generate<Vector3>(), generate<Vector3>(), generate<Vector3>()};
}

std::vector<Vector3> osc::tests::generate_triangle_vertices()
{
    return generate_into_vector(30, generate<Vector3>);
}

std::vector<Vector3> osc::tests::generate_vertices(size_t n)
{
    return generate_into_vector(n, generate<Vector3>);
}

std::vector<Vector3> osc::tests::generate_normals(size_t n)
{
    return generate_into_vector(n, []{ return normalize(generate<Vector3>()); });
}

std::vector<Vector2> osc::tests::generate_texture_coordinates(size_t n)
{
    return generate_into_vector(n, generate<Vector2>);
}

std::vector<Color> osc::tests::generate_colors(size_t n)
{
    return generate_into_vector(n, generate<Color>);
}

std::vector<Vector4> osc::tests::generate_tangent_vectors(size_t n)
{
    return generate_into_vector(n, generate<Vector4>);
}

std::vector<uint16_t> osc::tests::iota_index_range(size_t start, size_t end)
{
    std::vector<uint16_t> rv(end - start);
    std::iota(rv.begin(), rv.end(), static_cast<uint16_t>(start));
    return rv;
}

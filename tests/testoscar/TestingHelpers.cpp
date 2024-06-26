#include "TestingHelpers.h"

#include <oscar/Maths/MathHelpers.h>

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
}

std::default_random_engine& osc::testing::get_process_random_engine()
{
    // the RNG is deliberately deterministic, so that
    // test errors are reproducible
    static std::default_random_engine e{};  // NOLINT(cert-msc32-c,cert-msc51-cpp)
    return e;
}

template<> float osc::testing::generate()
{
    return static_cast<float>(std::uniform_real_distribution{}(get_process_random_engine()));
}

template<> int osc::testing::generate()
{
    return std::uniform_int_distribution{}(get_process_random_engine());
}

template<> bool osc::testing::generate()
{
    return generate<int>() % 2 == 0;
}

template<> uint8_t osc::testing::generate()
{
    return static_cast<uint8_t>(std::uniform_int_distribution{0, 255}(get_process_random_engine()));
}

template<> Color osc::testing::generate()
{
    return Color{generate<float>(), generate<float>(), generate<float>(), generate<float>()};
}

template<> Color32 osc::testing::generate()
{
    return {generate<uint8_t>(), generate<uint8_t>(), generate<uint8_t>(), generate<uint8_t>()};
}

template<> Vec2 osc::testing::generate()
{
    return Vec2{generate<float>(), generate<float>()};
}

template<> Vec3 osc::testing::generate()
{
    return Vec3{generate<float>(), generate<float>(), generate<float>()};
}

template<> Vec4 osc::testing::generate()
{
    return Vec4{generate<float>(), generate<float>(), generate<float>(), generate<float>()};
}

template<> Mat3 osc::testing::generate()
{
    return Mat3{generate<Vec3>(), generate<Vec3>(), generate<Vec3>()};
}

template<> Mat4 osc::testing::generate()
{
    return Mat4{generate<Vec4>(), generate<Vec4>(), generate<Vec4>(), generate<Vec4>()};
}

template<> Triangle osc::testing::generate()
{
    return Triangle{generate<Vec3>(), generate<Vec3>(), generate<Vec3>()};
}

std::vector<Vec3> osc::testing::generate_triangle_vertices()
{
    return generate_into_vector(30, generate<Vec3>);
}

std::vector<Vec3> osc::testing::generate_vertices(size_t n)
{
    return generate_into_vector(n, generate<Vec3>);
}

std::vector<Vec3> osc::testing::generate_normals(size_t n)
{
    return generate_into_vector(n, []() { return normalize(generate<Vec3>()); });
}

std::vector<Vec2> osc::testing::generate_texture_coordinates(size_t n)
{
    return generate_into_vector(n, generate<Vec2>);
}

std::vector<Color> osc::testing::generate_colors(size_t n)
{
    return generate_into_vector(n, generate<Color>);
}

std::vector<Vec4> osc::testing::generate_tangent_vectors(size_t n)
{
    return generate_into_vector(n, generate<Vec4>);
}

std::vector<uint16_t> osc::testing::iota_index_range(size_t start, size_t end)
{
    std::vector<uint16_t> rv(end - start);
    std::iota(rv.begin(), rv.end(), static_cast<uint16_t>(start));
    return rv;
}

#include "TestingHelpers.hpp"

#include <oscar/Maths/MathHelpers.hpp>

#include <random>

using osc::Color;
using osc::Normalize;
using osc::Mat3;
using osc::Mat4;
using osc::Vec2;
using osc::Vec3;
using osc::Vec4;

namespace
{
    template<class Generator>
    auto GenerateVector(size_t n, Generator f)
    {
        std::vector<decltype(f())> rv;
        rv.reserve(n);
        for (size_t i = 0; i < n; ++i)
        {
            rv.push_back(f());
        }
        return rv;
    }
}

std::default_random_engine& osc::testing::GetRngEngine()
{
    // the RNG is deliberately deterministic, so that
    // test errors are reproducible
    static std::default_random_engine e{};  // NOLINT(cert-msc32-c,cert-msc51-cpp)
    return e;
}

float osc::testing::GenerateFloat()
{
    return static_cast<float>(std::uniform_real_distribution{}(GetRngEngine()));
}

int osc::testing::GenerateInt()
{
    return std::uniform_int_distribution{}(GetRngEngine());
}

bool osc::testing::GenerateBool()
{
    return GenerateInt() % 2 == 0;
}

Color osc::testing::GenerateColor()
{
    return Color{GenerateFloat(), GenerateFloat(), GenerateFloat(), GenerateFloat()};
}

Vec2 osc::testing::GenerateVec2()
{
    return Vec2{GenerateFloat(), GenerateFloat()};
}

Vec3 osc::testing::GenerateVec3()
{
    return Vec3{GenerateFloat(), GenerateFloat(), GenerateFloat()};
}

Vec4 osc::testing::GenerateVec4()
{
    return Vec4{GenerateFloat(), GenerateFloat(), GenerateFloat(), GenerateFloat()};
}

Mat3 osc::testing::GenerateMat3x3()
{
    return Mat3{GenerateVec3(), GenerateVec3(), GenerateVec3()};
}

Mat4 osc::testing::GenerateMat4x4()
{
    return Mat4{GenerateVec4(), GenerateVec4(), GenerateVec4(), GenerateVec4()};
}

std::vector<Vec3> osc::testing::GenerateTriangleVerts()
{
    return GenerateVector(30, GenerateVec3);
}

std::vector<Vec3> osc::testing::GenerateVertices(size_t n)
{
    return GenerateVector(n, GenerateVec3);
}

std::vector<Vec3> osc::testing::GenerateNormals(size_t n)
{
    return GenerateVector(n, []() { return Normalize(GenerateVec3()); });
}

std::vector<Vec2> osc::testing::GenerateTexCoords(size_t n)
{
    return GenerateVector(n, GenerateVec2);
}

std::vector<Color> osc::testing::GenerateColors(size_t n)
{
    return GenerateVector(n, GenerateColor);
}

std::vector<Vec4> osc::testing::GenerateTangents(size_t n)
{
    return GenerateVector(n, GenerateVec4);
}

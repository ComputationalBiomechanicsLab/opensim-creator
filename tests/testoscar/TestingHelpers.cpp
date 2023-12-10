#include "TestingHelpers.hpp"

#include <random>

using osc::Color;
using osc::Mat3;
using osc::Mat4;
using osc::Vec2;
using osc::Vec3;
using osc::Vec4;

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
    std::vector<Vec3> rv;
    for (size_t i = 0; i < 30; ++i)
    {
        rv.push_back(GenerateVec3());
    }
    return rv;
}

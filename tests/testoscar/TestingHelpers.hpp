#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Mat3.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Utils/Concepts.hpp>

#include <algorithm>
#include <random>
#include <vector>

namespace osc::testing
{
    std::default_random_engine& GetRngEngine();

    float GenerateFloat();
    int GenerateInt();
    bool GenerateBool();
    Color GenerateColor();
    Vec2 GenerateVec2();
    Vec3 GenerateVec3();
    Vec4 GenerateVec4();
    Mat3 GenerateMat3x3();
    Mat4 GenerateMat4x4();
    std::vector<Vec3> GenerateTriangleVerts();

    template<ContiguousContainer T, ContiguousContainer U>
    bool ContainersEqual(T const& a, U const& b)
    {
        return std::equal(a.begin(), a.end(), b.begin(), b.end());
    }
}

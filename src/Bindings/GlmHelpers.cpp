#include "GlmHelpers.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, glm::vec2 const& v)
{
    return o << "vec2(" << v.x << ", " << v.y << ')';
}

std::ostream& osc::operator<<(std::ostream& o, glm::vec3 const& v)
{
    return o << "vec3(" << v.x << ", " << v.y << ", " << v.z << ')';
}

std::ostream& osc::operator<<(std::ostream& o, glm::vec4 const& v)
{
    return o << "vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')';
}

std::ostream& osc::operator<<(std::ostream& o, glm::mat3 const& m)
{
    // prints in row-major, because that's how most people debug matrices
    for (int row = 0; row < 3; ++row)
    {
        char const* delim = "";
        for (int col = 0; col < 3; ++col)
        {
            o << delim << m[col][row];
            delim = " ";
        }
        o << '\n';
    }
    return o;
}

std::ostream& osc::operator<<(std::ostream& o, glm::mat4x3 const& m)
{
    // prints in row-major, because that's how most people debug matrices
    for (int row = 0; row < 3; ++row)
    {
        char const* delim = "";
        for (int col = 0; col < 4; ++col)
        {
            o << delim << m[col][row];
            delim = " ";
        }
        o << '\n';
    }
    return o;
}

std::ostream& osc::operator<<(std::ostream& o, glm::mat4 const& m)
{
    // prints in row-major, because that's how most people debug matrices
    for (int row = 0; row < 4; ++row) {
        char const* delim = "";
        for (int col = 0; col < 4; ++col)
        {
            o << delim << m[col][row];
            delim = " ";
        }
        o << '\n';
    }
    return o;
}

std::ostream& osc::operator<<(std::ostream& o, glm::quat const& q)
{
    return o << "quat(x = " << q.x << ", y = " << q.y << ", z = " << q.z << ", w = " << q.w << ')';
}

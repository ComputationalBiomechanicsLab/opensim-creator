#include "glm_extensions.hpp"

#include <iostream>

std::ostream& glm::operator<<(std::ostream& o, vec3 const& v) {
    return o << "[" << v.x << ", " << v.y << ", " << v.z << "]";
}

std::ostream& glm::operator<<(std::ostream& o, vec4 const& v) {
    return o << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
}

std::ostream& glm::operator<<(std::ostream& o, mat4 const& m) {
    o << "[";
    for (auto i = 0U; i < 3; ++i) {
        o << m[i];
        o << ", ";
    }
    o << m[3];
    o << "]";
    return o;
}

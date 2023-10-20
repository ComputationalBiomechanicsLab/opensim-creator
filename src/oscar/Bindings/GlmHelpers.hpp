#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>

namespace osc
{
    // glm printing utilities - handy for debugging
    std::ostream& operator<<(std::ostream&, glm::vec2 const&);
    std::string to_string(glm::vec2 const&);

    std::ostream& operator<<(std::ostream&, glm::vec3 const&);
    std::string to_string(glm::vec3 const&);

    std::ostream& operator<<(std::ostream&, glm::vec4 const&);
    std::ostream& operator<<(std::ostream&, glm::mat3 const&);
    std::ostream& operator<<(std::ostream&, glm::mat4x3 const&);
    std::ostream& operator<<(std::ostream&, glm::mat4 const&);
    std::ostream& operator<<(std::ostream&, glm::quat const&);
}

template<>
struct std::hash<glm::vec3> final {
    size_t operator()(glm::vec3 const&);
};

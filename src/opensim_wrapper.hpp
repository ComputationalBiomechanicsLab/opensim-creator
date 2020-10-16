#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iosfwd>
#include <vector>
#include <variant>

// opensim_wrapper: wrapper code for OpenSim
//
// Main motivation for this is to compiler-firewall OpenSim away from the rest
// of the UI because it has atrocious translation-unit sizes (e.g. it takes
// 8 sec on my machine to compile one unit)

namespace osim {
    struct Cylinder final {
        glm::mat4 transform;
        glm::mat4 normal_xform;
        glm::vec4 rgba;
    };

    struct Line final {
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec4 rgba;
    };
    std::ostream& operator<<(std::ostream& o, osim::Line const& l);

    struct Point final {
    };

    struct Brick final {
    };

    struct Circle final {
    };

    struct Sphere final {
        glm::mat4 transform;
        glm::mat4 normal_xform;
        glm::vec4 rgba;
    };
    std::ostream& operator<<(std::ostream& o, osim::Sphere const& s);

    struct Ellipsoid final {
    };

    struct Frame final {
    };

    struct Text final {
    };

    struct Triangle {
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 p3;
    };

    struct Mesh final {
        glm::mat4 transform;
        glm::mat4 normal_xform;
        glm::vec4 rgba;
        std::vector<Triangle> triangles;
    };
    std::ostream& operator<<(std::ostream& o, osim::Mesh const& m);

    struct Arrow final {
    };

    struct Torus final {
    };

    struct Cone final {
    };

    using Geometry = std::variant<
        Cylinder,
        Line,
        Sphere,
        Mesh
    >;
    std::ostream& operator<<(std::ostream& o, osim::Geometry const& g);

    std::vector<Geometry> geometry_in(char const* model_path);
}

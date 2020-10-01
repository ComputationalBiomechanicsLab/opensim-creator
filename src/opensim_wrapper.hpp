#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <string_view>
#include <vector>
#include <variant>

namespace osim {
    struct Cylinder final {
        glm::mat4 transform;
        glm::vec3 scale;
        glm::vec4 rgba;
    };

    struct Line final {
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec4 rgba;
    };

    struct Point final {
    };

    struct Brick final {
    };

    struct Circle final {
    };

    struct Sphere final {
        glm::mat4 transform;
        glm::vec4 rgba;
        float radius;
    };

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
        glm::vec3 scale;
        glm::vec4 rgba;
        std::vector<Triangle> triangles;
    };

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

    std::vector<Geometry> geometry_in(std::string_view model_path);
}

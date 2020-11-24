#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iosfwd>
#include <vector>
#include <memory>

// opensim_wrapper: wrapper code for OpenSim
//
// Main motivation for this is to compiler-firewall OpenSim away from the rest
// of the UI because OpenSim headers have atrociously bad compile times

namespace osim {
    struct Model_handle;
    struct Model_wrapper final {
        std::shared_ptr<Model_handle> handle;
        ~Model_wrapper() noexcept;
    };

    Model_wrapper load_osim(char const* path);

    struct State_handle;
    struct State_wrapper final {
        std::unique_ptr<State_handle> handle;
        ~State_wrapper() noexcept;
    };

    State_wrapper initial_state(Model_wrapper&);

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

    struct Sphere final {
        glm::mat4 transform;
        glm::mat4 normal_xform;
        glm::vec4 rgba;
    };
    std::ostream& operator<<(std::ostream& o, osim::Sphere const& s);

    using Mesh_id = int;

    struct Mesh_instance final {
        glm::mat4 transform;
        glm::mat4 normal_xform;
        glm::vec4 rgba;

        Mesh_id mesh;
    };
    std::ostream& operator<<(std::ostream& o, osim::Mesh_instance const& m);

    struct State_geometry final {
        std::vector<Cylinder> cylinders;
        std::vector<Line> lines;
        std::vector<Sphere> spheres;
        std::vector<Mesh_instance> mesh_instances;
    };

    struct Triangle final {
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 p3;
    };

    struct Mesh final {
        std::vector<Triangle> triangles;
    };

    struct Geometry_loader_impl;
    class Geometry_loader final {
        std::unique_ptr<Geometry_loader_impl> impl;

    public:
        Geometry_loader();
        Geometry_loader(Geometry_loader const&) = delete;
        Geometry_loader(Geometry_loader&&);

        Geometry_loader& operator=(Geometry_loader const&) = delete;
        Geometry_loader& operator=(Geometry_loader&&);

        void geometry_in(State_wrapper const& state, State_geometry& out);
        std::string const& path_to(Mesh_id mesh) const;

        ~Geometry_loader() noexcept;
    };

    struct Mesh_loader_impl;
    class Mesh_loader final {
        std::unique_ptr<Mesh_loader_impl> impl;

    public:
        Mesh_loader();
        Mesh_loader(Mesh_loader const&) = delete;
        Mesh_loader(Mesh_loader&&);

        Mesh_loader& operator=(Mesh_loader const&) = delete;
        Mesh_loader& operator=(Mesh_loader&&);

        void load(std::string const& path, Mesh& out);

        ~Mesh_loader() noexcept;
    };
}

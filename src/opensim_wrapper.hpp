#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iosfwd>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>

namespace OpenSim {
    class Model;
}

namespace SimTK {
    class State;
}

// opensim_wrapper: wrapper code for OpenSim
//
// Main motivation for this header is to compiler-firewall OpenSim away from
// the rest of the codebase because OpenSim.h is massive and can increase
// compile times by *a lot*
namespace osim {
    // initialization + loading

    // RAII wrapper for an OpenSim model
    struct OSMV_Model final {
        std::unique_ptr<OpenSim::Model> handle;

        OSMV_Model(std::unique_ptr<OpenSim::Model>);
        OSMV_Model(OSMV_Model const&) = delete;
        OSMV_Model(OSMV_Model&&) noexcept;
        OSMV_Model& operator=(OSMV_Model const&) = delete;
        OSMV_Model& operator=(OSMV_Model&&) noexcept;
        ~OSMV_Model() noexcept;

        OpenSim::Model& operator*() noexcept {
            return *handle;
        }

        OpenSim::Model const& operator*() const noexcept {
            return *handle;
        }

        OpenSim::Model* operator->() noexcept {
            return handle.get();
        }

        OpenSim::Model const* operator->() const noexcept {
            return handle.get();
        }
    };

    // opaque lifetime wrapper for a SimTK state
    struct OSMV_State final {
        std::unique_ptr<SimTK::State> handle;

        OSMV_State();
        OSMV_State(std::unique_ptr<SimTK::State>);
        OSMV_State(OSMV_State const&) = delete;
        OSMV_State(OSMV_State&&) noexcept;
        OSMV_State& operator=(OSMV_State const&) = delete;
        OSMV_State& operator=(OSMV_State&&) noexcept;
        ~OSMV_State() noexcept;

        operator bool() const noexcept {
            return handle.operator bool();
        }

        SimTK::State& operator*() noexcept {
            return *handle;
        }

        SimTK::State const& operator*() const noexcept {
            return *handle;
        }

        SimTK::State* operator->() noexcept {
            return handle.get();
        }

        SimTK::State const* operator->() const noexcept {
            return handle.get();
        }
    };


    // simplifed loading + simulating API for OpenSim

    OSMV_Model load_osim(char const* path);
    OSMV_Model copy_model(OpenSim::Model const&);
    SimTK::State& init_system(OpenSim::Model&);
    SimTK::State& upd_working_state(OpenSim::Model&);
    void finalize_properties_from_state(OpenSim::Model&, SimTK::State&);

    OSMV_State copy_state(SimTK::State const&);
    void realize_position(OpenSim::Model&, SimTK::State&);
    void realize_report(OpenSim::Model&, SimTK::State&);
    OSMV_State fd_simulation(
            OpenSim::Model& model,
            SimTK::State const& initial_state,
            double final_time,
            std::function<void(SimTK::State const&)> reporter);


    // rendering API
    //
    // main motivation here is to extract renderable geometry from a
    // model + state pair. Main considerations:
    //
    // - must be relatively high-perf because the renderer may call it many
    //   times per second
    //
    //     - so the structs and methods are designed to minimize allocations
    //       and be easy to perform comparisons/hashing on
    //
    // - provide enough information to the renderer for it to link renderable
    //   geometry to components in the OpenSim model
    //
    //     - i.e. when a mouse is over a muscle, the renderer must be able to
    //       handle that in a useful way
    //
    // - provide enough geometric information for extra optimization passes
    //
    //     - e.g. bounding boxes, normals, locations in worldspace, deduped
    //       mesh IDs for mesh instancing, etc. etc.

    using Mesh_id = int;

    struct Mesh_instance final {
        glm::mat4 transform;
        glm::mat4 normal_xform;
        glm::vec4 rgba;

        Mesh_id mesh;
    };
    std::ostream& operator<<(std::ostream& o, osim::Mesh_instance const& m);

    struct State_geometry final {
        std::vector<Mesh_instance> mesh_instances;
    };


    struct Untextured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
    };

    struct Untextured_triangle final {
        Untextured_vert p1;
        Untextured_vert p2;
        Untextured_vert p3;
    };

    struct Untextured_mesh final {
        std::vector<Untextured_triangle> triangles;
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

        void geometry_in(OpenSim::Model& model, SimTK::State& s, State_geometry& out);
        void load_mesh(Mesh_id id, Untextured_mesh& out);

        ~Geometry_loader() noexcept;
    };
}

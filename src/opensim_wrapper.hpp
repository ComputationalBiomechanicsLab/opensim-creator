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
    class Coordinate;
    class Muscle;
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

    // opaque RAII wrapper OpenSim::Model
    struct OSMV_Model final {
        std::unique_ptr<OpenSim::Model> handle;

        OSMV_Model(std::unique_ptr<OpenSim::Model>);
        OSMV_Model(OSMV_Model const&) = delete;
        OSMV_Model(OSMV_Model&&) noexcept;
        OSMV_Model& operator=(OSMV_Model const&) = delete;
        OSMV_Model& operator=(OSMV_Model&&) noexcept;
        ~OSMV_Model() noexcept;

        operator OpenSim::Model const&() const noexcept {
            return *handle;
        }

        operator OpenSim::Model&() noexcept {
            return *handle;
        }
    };

    // opaque RAII wrapper for SimTK::State
    struct OSMV_State final {
        std::unique_ptr<SimTK::State> handle;

        OSMV_State(std::unique_ptr<SimTK::State>);
        OSMV_State(SimTK::State const&);  // copies
        OSMV_State(OSMV_State const&) = delete;
        OSMV_State(OSMV_State&&) noexcept;
        OSMV_State& operator=(OSMV_State const&) = delete;
        OSMV_State& operator=(SimTK::State const&); // copies
        OSMV_State& operator=(OSMV_State&&) noexcept;
        ~OSMV_State() noexcept;

        operator SimTK::State const&() const noexcept {
            assert(handle != nullptr);
            return *handle;
        }

        operator SimTK::State&() noexcept {
            assert(handle != nullptr);
            return *handle;
        }

        operator bool() const noexcept {
            return handle != nullptr;
        }
    };

    // emitted by simulations as they run
    struct Simulation_update_event final {
        SimTK::State const& state;
        double simulation_time;
        int num_prescribe_q_calls;
    };

    // translation for OpenSim::Coordinate::MotionType that can be ORed easily
    // for filtering
    enum Motion_type : int {
        Undefined = 0,
        Rotational = 1,
        Translational = 2,
        Coupled = 4
    };

    // coordinates in a model
    struct Coordinate final {
        OpenSim::Coordinate const* ptr;
        std::string const* name;
        float min;
        float max;
        float value;
        Motion_type type;
        bool locked;
    };

    struct Muscle_stat final {
        OpenSim::Muscle const* ptr;
        std::string const* name;
        float length;
    };

    // simplified API to OpenSim. Users that want something more advanced
    // should directly #include OpenSim's headers (but eat the compile times)

    OSMV_Model load_osim(char const* path);
    void finalize_from_properties(OpenSim::Model&);
    OSMV_Model copy_model(OpenSim::Model const&);
    SimTK::State& init_system(OpenSim::Model&);
    SimTK::State& upd_working_state(OpenSim::Model&);
    void finalize_properties_from_state(OpenSim::Model&, SimTK::State const&);
    double simulation_time(SimTK::State const&);
    void get_coordinates(OpenSim::Model const&, SimTK::State const&, std::vector<Coordinate>&);
    void get_muscle_stats(OpenSim::Model const&, SimTK::State const&, std::vector<Muscle_stat>&);
    void set_coord_value(OpenSim::Coordinate const&, SimTK::State&, double);
    void lock_coord(OpenSim::Coordinate const&, SimTK::State&);
    void unlock_coord(OpenSim::Coordinate const&, SimTK::State&);
    void disable_wrapping_surfaces(OpenSim::Model&);
    void enable_wrapping_surfaces(OpenSim::Model&);

    void compute_moment_arms(
            OpenSim::Muscle const&,
            SimTK::State const&,
            OpenSim::Coordinate const&,
            float* out,
            size_t steps);

    void realize_report(OpenSim::Model const&, SimTK::State&);
    void realize_velocity(OpenSim::Model&, SimTK::State&);
    OSMV_State fd_simulation(
            OpenSim::Model& model,
            SimTK::State const& initial_state,
            double final_time,
            std::function<int(Simulation_update_event const&)> reporter);


    // high-level rendering API

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

        void clear() {
            mesh_instances.clear();
        }
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

        void all_geometry_in(OpenSim::Model& model, SimTK::State& s, State_geometry& out);
        void load_mesh(Mesh_id id, Untextured_mesh& out);

        ~Geometry_loader() noexcept;
    };
}

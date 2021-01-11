#pragma once

#include "3d_common.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <filesystem>

namespace OpenSim {
    class Model;
    class Coordinate;
    class Muscle;
    class AbstractOutput;
}

namespace SimTK {
    class State;
}

// opensim_wrapper: wrapper code for OpenSim
//
// Main motivation for this header is to compiler-firewall OpenSim away from
// the rest of the codebase because OpenSim.h is massive and can increase
// compile times by *a lot*
namespace osmv {

    // owned (but opaque) handle to an OpenSim::Model
    struct Model final {
        std::unique_ptr<OpenSim::Model> handle;

        explicit Model(std::unique_ptr<OpenSim::Model>) noexcept;
        explicit Model(OpenSim::Model const&);
        Model(Model const&) = delete;
        Model(Model&&) noexcept;
        Model& operator=(Model const&) = delete;
        Model& operator=(Model&&) noexcept;
        ~Model() noexcept;

        operator OpenSim::Model const&() const noexcept {
            return *handle;
        }

        operator OpenSim::Model&() noexcept {
            return *handle;
        }
    };

    // owned (but opaque) handle to a SimTK::State
    struct State final {
        std::unique_ptr<SimTK::State> handle;

        explicit State(SimTK::State const&);
        State(State const&) = delete;
        State(State&&) noexcept;
        State& operator=(State const&) = delete;
        State& operator=(SimTK::State const&);
        State& operator=(State&&) noexcept;
        ~State() noexcept;

        operator SimTK::State const&() const noexcept {
            return *handle;
        }

        operator SimTK::State&() noexcept {
            return *handle;
        }
    };

    // response from integration step callback
    enum class Callback_response {
        Ok,               // callback executed ok
        Please_halt       // callback wants the simulator to halt
    };

    // top-level configuration for a basic forward-dynamic sim
    struct Fd_sim_config final {
        double final_time = 0.4;
        int max_steps = 20000;
        double min_step_size = 1.0e-8;
        double max_step_size = 1.0;
        double integrator_accuracy = 1.0e-5;
        std::optional<std::function<Callback_response(SimTK::State const&)>> on_integration_step = std::nullopt;
    };

    // flag-ified version of OpenSim::Coordinate::MotionType (easier ORing for filtering)
    enum Motion_type : int {
        Undefined = 0,
        Rotational = 1,
        Translational = 2,
        Coupled = 4,
    };
    static_assert(Motion_type::Undefined == 0);

    // info for a coordinate in a model
    //
    // pointers in this struct are dependent on the model: only use this in short-lived contexts
    // and don't let it survive during a model edit or model destruction
    struct Coordinate final {
        OpenSim::Coordinate const* ptr;
        std::string const* name;
        float min;
        float max;
        float value;
        Motion_type type;
        bool locked;
    };

    // info for a muscle in a model
    //
    // pointers in this struct are dependent on the model: only use this in short-lived contexts
    // and don't let it survive during a model edit or model destruction
    struct Muscle_stat final {
        OpenSim::Muscle const* ptr;
        std::string const* name;
        float length;
    };

    // info for a (data) output declared by the model
    //
    // pointers in this struct are dependent on the model: only use this in short-lived contexts
    // and don't let it survive during a model edit or model destruction
    struct Available_output final {
        std::string const* owner_name;
        std::string const* output_name;
        OpenSim::AbstractOutput const* handle;
        bool is_single_double_val;
    };

    inline bool operator==(Available_output const& a, Available_output const& b) {
        return a.handle == b.handle;
    }


    // OpenSim API WRAPPING FUNCTIONS:
    //
    // - simplified API to OpenSim
    //
    // - designed to be fast to compile, so UI development doesn't have to eat the 10+ second
    //   compile times that #include'ing OpenSim.h can cause
    //
    // - many of these functions are potentially called as part of a draw call (e.g. *very*
    //   frequently), so many of these functions use outparams and other
    //   memory-allocation-minimization tricks
    //
    // - If you need direct OpenSim API access, put what you need into a separate (from the UI)
    //   compilation unit. The wrappers above (e.g. OSMV_Model) can be used directly with the raw
    //   OpenSim API (e.g. OSMV_Model implicitly converts into OpenSim::Model&)


    Model load_osim(std::filesystem::path const&);
    void finalize_from_properties(OpenSim::Model&);
    SimTK::State& init_system(OpenSim::Model&);
    SimTK::State& upd_working_state(OpenSim::Model&);
    void finalize_properties_from_state(OpenSim::Model&, SimTK::State const&);
    void realize_report(OpenSim::Model const&, SimTK::State&);
    void realize_velocity(OpenSim::Model const&, SimTK::State&);

    void get_coordinates(OpenSim::Model const&, SimTK::State const&, std::vector<Coordinate>& out);
    void get_muscle_stats(OpenSim::Model const&, SimTK::State const&, std::vector<Muscle_stat>& out);
    void set_coord_value(OpenSim::Coordinate const&, SimTK::State&, double);
    void lock_coord(OpenSim::Coordinate const&, SimTK::State&);
    void unlock_coord(OpenSim::Coordinate const&, SimTK::State&);
    void disable_wrapping_surfaces(OpenSim::Model&);
    void enable_wrapping_surfaces(OpenSim::Model&);

    // *out is assumed to point to an area of memory with space to hold `steps` datapoints
    void compute_moment_arms(
            OpenSim::Muscle const&,
            SimTK::State const&,
            OpenSim::Coordinate const&,
            float* out,
            size_t steps);

    void get_available_outputs(OpenSim::Model const&, std::vector<Available_output>& out);
    std::string get_output_val_any(OpenSim::AbstractOutput const&, SimTK::State const&);
    double get_output_val_double(OpenSim::AbstractOutput const&, SimTK::State const&);

    // run a forward-dynamic simulation of model, starting from the specified initial state, with
    // specified config
    //
    // returns the final state of the simulation (i.e. the state of the last integration step)
    State fd_simulation(OpenSim::Model& model, State initial_state, Fd_sim_config const& config);

    // run a forward-dynamic simulation of model, using the model's initial state (from init_system)
    // and default simulation config
    State fd_simulation(OpenSim::Model&);

    double simulation_time(SimTK::State const&);
    int num_prescribeq_calls(OpenSim::Model const&);



    // RENDERING

    // mesh IDs are guaranteed to be globally unique and monotonically increase from 1
    //
    // this guarantee is important because it means that calling code can use direct integer index
    // lookups, rather than having to rely on (e.g.) a runtime hashtable
    using Mesh_id = size_t;

    // one instance of a mesh
    //
    // a model may contain multiple instances of the same mesh
    struct Mesh_instance final {
        glm::mat4 transform;
        glm::mat4 normal_xform;
        glm::vec4 rgba;

        Mesh_id mesh_id;
    };

    // for this API, an OpenSim model's geometry is described as a sequence of rgba-colored mesh
    // instances that are transformed into world coordinates
    struct State_geometry final {
        std::vector<Mesh_instance> mesh_instances;

        void clear() {
            mesh_instances.clear();
        }
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
        void load_mesh(Mesh_id id, std::vector<Untextured_vert>& out);

        ~Geometry_loader() noexcept;
    };
}

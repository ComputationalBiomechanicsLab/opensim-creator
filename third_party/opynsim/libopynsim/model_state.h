#pragma once

#include <libopynsim/model_state_stage.h>
#include <libopynsim/symbol.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <unordered_map>

namespace opyn { class Model; }
namespace SimTK { class State; }

namespace opyn
{
    // Represents all state information for a `Model`.
    //
    // Related: https://simtk.org/api_docs/simbody/3.5/classSimTK_1_1State.html#details
    // Related: https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53089017/SimTK+Simulation+Concepts
    class ModelState final {
    public:
        using attrs_type = std::unordered_map<Symbol, double>;

        /// Constructs an empty model state.
        ModelState();

        explicit ModelState(SimTK::State&&);

        /// Returns the `ModelStateStage` that this state has been realized to.
        ModelStateStage stage() const;

        /// Returns the time point, in seconds, that this state represents.
        double time() const;

        /// Returns this `ModelState`'s metadata (e.g. integrator stats, performance counters).
        const attrs_type& attrs() const;

        /// Sets the `attrs` of this `ModelState` to `new_attrs`.
        void set_attrs(attrs_type new_attrs);

        /// Returns the underlying `SimTK::State` (internal method: be careful with this).
        const SimTK::State& simbody_state() const;

        /// Returns the underlying `SimTK::State` (internal method: be careful with this).
        SimTK::State& upd_simbody_state();

    private:
        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}

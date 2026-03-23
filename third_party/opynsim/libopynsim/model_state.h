#pragma once

#include <libopynsim/model_state_stage.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>

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
        ModelStateStage stage() const;
    private:
        friend class Model;
        explicit ModelState(SimTK::State&&);

        const SimTK::State& simbody_state() const;
        SimTK::State& simbody_state();

        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}

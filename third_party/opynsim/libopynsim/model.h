#pragma once

#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>

namespace OpenSim { class Model; }

namespace opyn
{
    // Represents a readonly (`const`) multibody physics system compiled from
    // a `ModelSpecification`.
    class Model final {
    private:
        friend class ModelSpecification;
        explicit Model(const OpenSim::Model&);

    public:
        ModelState initial_state() const;
        void realize(ModelState&, ModelStateStage) const;

        const OpenSim::Model& opensim_model() const;
    private:
        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}

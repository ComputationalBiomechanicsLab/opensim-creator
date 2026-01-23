#pragma once

#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>

#include <liboscar/utils/copy_on_upd_ptr.h>

namespace OpenSim { class Model; }

namespace opyn
{
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

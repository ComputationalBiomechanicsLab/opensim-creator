#include "model.h"

#include <OpenSim/Simulation/Model/Model.h>
#include <libopynsim/model_state.h>
#include <liboscar/utilities/copy_on_upd_ptr.h>

class opyn::Model::Impl final {
public:
    explicit Impl(const OpenSim::Model& model) :
        model_{model}
    {
        // This is effectively what converting a "model specification" to
        // a "model" is, in `opynsim`'s world.
        model_.buildSystem();

        // This is a quirk of OpenSim, because it mixes the state and system
        // into one class, but it must be done here because `initial_state()`
        // is `const` in `opynsim`'s design.
        model_.initializeState();
    }

    ModelState initial_state() const
    {
        // Copy the working state out of the model, so that the caller gets
        // an independent state.
        return ModelState{SimTK::State{model_.getWorkingState()}};
    }

    void realize(ModelState& state, ModelStateStage stage) const
    {
        switch (stage) {
        case ModelStateStage::time:         model_.realizeTime(state.simbody_state());         break;
        case ModelStateStage::position:     model_.realizePosition(state.simbody_state());     break;
        case ModelStateStage::velocity:     model_.realizeVelocity(state.simbody_state());     break;
        case ModelStateStage::dynamics:     model_.realizeDynamics(state.simbody_state());     break;
        case ModelStateStage::acceleration: model_.realizeAcceleration(state.simbody_state()); break;
        case ModelStateStage::report:       model_.realizeReport(state.simbody_state());       break;
        default:                            model_.realizeReport(state.simbody_state());       break;
        }
    }

    const OpenSim::Model& opensim_model() const { return model_; }
private:
    OpenSim::Model model_;
};

opyn::Model::Model(const OpenSim::Model& model) : impl_{osc::make_cow<Impl>(model)} {}
opyn::ModelState opyn::Model::initial_state() const { return impl_->initial_state(); }
void opyn::Model::realize(ModelState& state, ModelStateStage stage) const { impl_->realize(state, stage); }
const OpenSim::Model& opyn::Model::opensim_model() const { return impl_->opensim_model(); }

#include "model.h"

#include <OpenSim/Simulation/Model/Model.h>
#include <libopynsim/model_state.h>
#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

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

    size_t num_coordinates() const
    {
        const auto lst = model_.getComponentList<OpenSim::Coordinate>();
        return std::distance(lst.begin(), lst.end());
    }

    std::vector<Symbol> coordinates() const
    {
        std::vector<Symbol> rv;
        for (const auto& coordinate : model_.getComponentList<OpenSim::Coordinate>()) {
            rv.emplace_back(coordinate.getAbsolutePathString());
        }
        return rv;
    }

    double get_coordinate_value(const ModelState& model_state, const Symbol& coordinate) const
    {
        const auto& coord = model_.getComponent<OpenSim::Coordinate>(std::string{coordinate});
        return coord.getValue(model_state.simbody_state());
    }

    void set_coordinate_value(ModelState& model_state, const Symbol& coordinate, double value) const
    {
        const auto& coord = model_.getComponent<OpenSim::Coordinate>(std::string{coordinate});
        coord.setValue(model_state.simbody_state(), value);
    }

    size_t num_outputs() const
    {
        size_t rv = 0;
        for (const auto& component : model_.getComponentList()) {
            rv += component.getNumOutputs();
        }
        return rv;
    }

    std::vector<Symbol> outputs() const
    {
        std::vector<Symbol> rv;
        for (const auto& component : model_.getComponentList()) {
            const std::string abs_path = component.getAbsolutePathString();
            for (const auto& [name, output] : component.getOutputs()) {
                std::stringstream ss;
                ss << abs_path << '[' << name << ']';
                rv.emplace_back(std::move(ss).str());
            }
        }
        return rv;
    }

    const OpenSim::Model& opensim_model() const { return model_; }
private:
    OpenSim::Model model_;
};

opyn::Model::Model(const OpenSim::Model& model) :
    impl_{osc::make_cow<Impl>(model)}
{}

opyn::ModelState opyn::Model::initial_state() const
{
    return impl_->initial_state();
}

void opyn::Model::realize(ModelState& model_state, ModelStateStage model_state_stage) const
{
    impl_->realize(model_state, model_state_stage);
}

size_t opyn::Model::num_coordinates() const
{
    return impl_->num_coordinates();
}

std::vector<opyn::Symbol> opyn::Model::coordinates() const
{
    return impl_->coordinates();
}

double opyn::Model::get_coordinate_value(const ModelState& model_state, const Symbol& coordinate) const
{
    return impl_->get_coordinate_value(model_state, coordinate);
}

void opyn::Model::set_coordinate_value(ModelState& model_state, const Symbol& coordinate, double value) const
{
    impl_->set_coordinate_value(model_state, coordinate, value);
}

size_t opyn::Model::num_outputs() const
{
    return impl_->num_outputs();
}

std::vector<opyn::Symbol> opyn::Model::outputs() const
{
    return impl_->outputs();
}

opyn::OutputValue opyn::Model::get_output_value(const ModelState&, const Symbol&) const
{
    return "Hello, world";  // TODO
}

const OpenSim::Model& opyn::Model::opensim_model() const
{
    return impl_->opensim_model();
}

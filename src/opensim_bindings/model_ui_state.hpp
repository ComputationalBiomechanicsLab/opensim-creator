#pragma once

#include <memory>

#include <OpenSim/Simulation/Model/Model.h>

namespace osmv {

    // bundles together:
    //
    // - model
    // - state
    // - selection
    // - hover
    //
    // into a single class that supports coherent copying, moving, assingment etc.
    //
    // enables snapshotting everything necessary to render a typical UI scene (just copy this) and
    // should automatically update/invalidate any pointers, states, etc. on each operation
    struct Model_ui_state final {
        static std::unique_ptr<OpenSim::Model> copy_model(OpenSim::Model const& model) {
            auto copy = std::make_unique<OpenSim::Model>(model);
            copy->finalizeFromProperties();
            return copy;
        }

        static SimTK::State init_fresh_system_and_state(OpenSim::Model& model) {
            SimTK::State rv = model.initSystem();
            model.realizePosition(rv);
            return rv;
        }

        // relocate a component pointer to point into `model`, rather into whatever
        // model it used to point into
        static OpenSim::Component* relocate_pointer(OpenSim::Model const& model, OpenSim::Component* ptr) {
            if (!ptr) {
                return nullptr;
            }

            try {
                return const_cast<OpenSim::Component*>(model.findComponent(ptr->getAbsolutePath()));
            } catch (OpenSim::Exception const&) {
                // finding fails with exception when ambiguous (fml)
                return nullptr;
            }
        }

        std::unique_ptr<OpenSim::Model> model = nullptr;
        SimTK::State state = {};
        OpenSim::Component* selected_component = nullptr;
        OpenSim::Component* hovered_component = nullptr;

        Model_ui_state() = default;

        Model_ui_state(std::unique_ptr<OpenSim::Model> _model) :
            model{std::move(_model)},
            state{init_fresh_system_and_state(*model)},
            selected_component{nullptr},
            hovered_component{nullptr} {
        }

        Model_ui_state(Model_ui_state const& other) :
            model{copy_model(*other.model)},
            state{init_fresh_system_and_state(*model)},
            selected_component{relocate_pointer(*model, other.selected_component)},
            hovered_component{relocate_pointer(*model, other.hovered_component)} {
        }

        Model_ui_state(Model_ui_state&& tmp) :
            model{std::move(tmp.model)},
            state{std::move(tmp.state)},
            selected_component{std::move(tmp.selected_component)},
            hovered_component{std::move(tmp.hovered_component)} {
        }

        friend void swap(Model_ui_state& a, Model_ui_state& b) {
            std::swap(a.model, b.model);
            std::swap(a.state, b.state);
            std::swap(a.selected_component, b.selected_component);
            std::swap(a.hovered_component, b.hovered_component);
        }

        Model_ui_state& operator=(Model_ui_state other) {
            swap(*this, other);
            return *this;
        }

        Model_ui_state& operator=(std::unique_ptr<OpenSim::Model> ptr) {
            if (model == ptr) {
                return *this;
            }

            std::swap(model, ptr);
            state = init_fresh_system_and_state(*model);
            selected_component = relocate_pointer(*model, selected_component);
            hovered_component = relocate_pointer(*model, hovered_component);

            return *this;
        }

        // beware: can throw, because the model might've been put into an invalid
        // state by the modification
        void on_model_modified() {
            state = init_fresh_system_and_state(*model);
            selected_component = relocate_pointer(*model, selected_component);
            hovered_component = relocate_pointer(*model, hovered_component);
        }
    };
}

#pragma once

#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Utils/CircularBuffer.hpp"

#include <memory>
#include <optional>

namespace OpenSim {
    class Model;
    class Component;
}

namespace SimTK {
    class State;
}

namespace osc {
    // a "UI-ready" OpenSim::Model with undo/redo and rollback support
    //
    // this is what the top-level editor screens are managing. As the user makes
    // edits to the model, the current/undo/redo states are being updated. This
    // class also has light support for handling "rollbacks", which is where the
    // implementation detects that the user modified the model into an invalid state
    // and the implementation tried to fix the problem by rolling back to an earlier
    // (hopefully, valid) undo state
    struct UndoableUiModel final {
        UiModel current;
        CircularBuffer<UiModel, 32> undo;
        CircularBuffer<UiModel, 32> redo;

        // holding space for a "damaged" model
        //
        // this is set whenever the implementation detects that the current
        // model was damaged by a modification (i.e. the model does not survive a
        // call to .initSystem with its modified properties).
        //
        // The implementation will try to recover from the damage by popping models
        // from the undo buffer and making them `current`. It will then store the
        // damaged model here for later cleanup (by the user of this class, which
        // should `std::move` out the damaged instance)
        //
        // the damaged model is kept "alive" so that any pointers into the model are
        // still valid. The reason this is important is because the damage may have
        // been done midway through a larger process (e.g. rendering) and there may
        // be local (stack-allocated) pointers into the damaged model's components.
        // In that case, it is *probably* safer to let that process finish with a
        // damaged model than potentially segfault.
        std::optional<UiModel> damaged;

        // make a new undoable UI model
        UndoableUiModel();

        explicit UndoableUiModel(std::unique_ptr<OpenSim::Model> model);

        [[nodiscard]] constexpr bool canUndo() const noexcept {
            return !undo.empty();
        }

        void doUndo();

        [[nodiscard]] constexpr bool canRedo() const noexcept {
            return !redo.empty();
        }

        void doRedo();


        OpenSim::Model const& getModel() const noexcept {
            return current.getModel();
        }

        OpenSim::Model& updModel() noexcept {
            return current.updModel();
        }


        SimTK::State const& getState() const noexcept {
            return current.getState();
        }

        SimTK::State& updState() noexcept {
            return current.updState();
        }

        void setModel(std::unique_ptr<OpenSim::Model>);

        // this should be called before any modification is made to the current model
        //
        // it gives the implementation a chance to save a known-to-be-undamaged
        // version of `current` before any potential damage happens
        void beforeModifyingModel();

        // this should be called after any modification is made to the current model
        //
        // it "commits" the modification by calling `on_model_modified` on the model
        // in a way that will "rollback" if comitting the change throws an exception
        void afterModifyingModel();

        // tries to rollback the model to an earlier state, throwing an exception if
        // that isn't possible (e.g. because there are no earlier states)
        void forciblyRollbackToEarlierState();


        bool hasSelected() const {
            return current.getSelected() != nullptr;
        }

        OpenSim::Component const* getSelected() const {
            return current.getSelected();
        }

        OpenSim::Component* updSelected() {
            return current.updSelected();
        }

        void setSelected(OpenSim::Component const* c) {
            current.setSelected(c);
        }

        template<typename T>
        bool selectionIsType() const {
            return current.selectionIsType<T>();
        }

        template<typename T>
        bool selectionDerivesFrom() const {
            return current.selectionDerivesFrom<T>();
        }

        template<typename T>
        T const* getSelectedAs() const {
            return current.getSelectedAs<T>();
        }

        template<typename T>
        T* updSelectedAs() {
            return current.updSelectedAs<T>();
        }

        bool hasHovered() const {
            return current.hasHovered();
        }

        OpenSim::Component const* getHovered() const noexcept {
            return current.getHovered();
        }

        OpenSim::Component* updHovered() {
            return current.updHovered();
        }

        void setHovered(OpenSim::Component const* c) {
            current.setHovered(c);
        }


        OpenSim::Component const* getIsolated() const noexcept {
            return current.getIsolated();
        }

        OpenSim::Component* updIsolated() {
            return current.updIsolated();
        }

        void setIsolated(OpenSim::Component const* c) {
            current.setIsolated(c);
        }


        void clearAnyDamagedModels();

        // declare the death of a component pointer
        //
        // this happens when we know that OpenSim has destructed a component in
        // the model indirectly (e.g. it was destructed by an OpenSim container)
        // and that we want to ensure the pointer isn't still held by this state
        void declareDeathOf(OpenSim::Component const* c) noexcept {
            if (current.getSelected() == c) {
                current.setSelected(nullptr);
            }

            if (current.getHovered() == c) {
                current.setHovered(nullptr);
            }

            if (current.getIsolated() == c) {
                current.setIsolated(nullptr);
            }
        }
    };
}

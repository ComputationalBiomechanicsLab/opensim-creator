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
    // A "UI ready" model with undo/redo support
    //
    // contains four major points of interest:
    //
    //     a) an "active", potentially dirty, model
    //     b) a "committed", always clean, model
    //     c) an undo buffer containing clean models
    //     d) an redo buffer containing clean models
    //
    // - the editor can edit `a` at any time
    // - the UI thread regularly (i.e. once per frame) calls `a.updateIfDirty()`
    // - if updating succeeds, `b` is pushed into the undo buffer and `a` is
    //   *copied* into `b`
    // - if updating fails, `b` is copied into `a` as a "fallback" to ensure that
    //   `a` is left in a valid state
    class UndoableUiModel final {
        UiModel m_Current;
        CircularBuffer<UiModel, 32> m_UndoBuffer;
        CircularBuffer<UiModel, 32> m_RedoBuffer;

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
        std::optional<UiModel> m_Damaged;

        void rollbackModelToEarlierState();

    public:

        // make a new, blank, undoable model
        UndoableUiModel();

        // make a new undoable model from an existing in-memory OpenSim model
        explicit UndoableUiModel(std::unique_ptr<OpenSim::Model> model);

        UiModel const& getUiModel() const {
            return m_Current;
        }

        UiModel& updUiModel() {
            return m_Current;
        }

        [[nodiscard]] constexpr bool canUndo() const noexcept {
            return !m_UndoBuffer.empty();
        }

        void doUndo();

        [[nodiscard]] constexpr bool canRedo() const noexcept {
            return !m_RedoBuffer.empty();
        }

        void doRedo();


        OpenSim::Model const& getModel() const noexcept {
            return m_Current.getModel();
        }

        OpenSim::Model& updModel() noexcept {
            return m_Current.updModel();
        }


        SimTK::State const& getState() const noexcept {
            return m_Current.getState();
        }

        SimTK::State& updState() noexcept {
            return m_Current.updState();
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
            return m_Current.getSelected() != nullptr;
        }

        OpenSim::Component const* getSelected() const {
            return m_Current.getSelected();
        }

        OpenSim::Component* updSelected() {
            return m_Current.updSelected();
        }

        void setSelected(OpenSim::Component const* c) {
            m_Current.setSelected(c);
        }

        template<typename T>
        bool selectionIsType() const {
            return m_Current.selectionIsType<T>();
        }

        template<typename T>
        bool selectionDerivesFrom() const {
            return m_Current.selectionDerivesFrom<T>();
        }

        template<typename T>
        T const* getSelectedAs() const {
            return m_Current.getSelectedAs<T>();
        }

        template<typename T>
        T* updSelectedAs() {
            return m_Current.updSelectedAs<T>();
        }

        bool hasHovered() const {
            return m_Current.hasHovered();
        }

        OpenSim::Component const* getHovered() const noexcept {
            return m_Current.getHovered();
        }

        OpenSim::Component* updHovered() {
            return m_Current.updHovered();
        }

        void setHovered(OpenSim::Component const* c) {
            m_Current.setHovered(c);
        }


        OpenSim::Component const* getIsolated() const noexcept {
            return m_Current.getIsolated();
        }

        OpenSim::Component* updIsolated() {
            return m_Current.updIsolated();
        }

        void setIsolated(OpenSim::Component const* c) {
            m_Current.setIsolated(c);
        }


        void clearAnyDamagedModels();

        // declare the death of a component pointer
        //
        // this happens when we know that OpenSim has destructed a component in
        // the model indirectly (e.g. it was destructed by an OpenSim container)
        // and that we want to ensure the pointer isn't still held by this state
        void declareDeathOf(OpenSim::Component const* c) noexcept {
            if (m_Current.getSelected() == c) {
                m_Current.setSelected(nullptr);
            }

            if (m_Current.getHovered() == c) {
                m_Current.setHovered(nullptr);
            }

            if (m_Current.getIsolated() == c) {
                m_Current.setIsolated(nullptr);
            }
        }
    };
}

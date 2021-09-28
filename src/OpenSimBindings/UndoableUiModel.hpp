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
    //     b) a "copy"/"committed", always clean, model
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
    public:
        // make a new, blank, undoable model
        UndoableUiModel();

        // make a new undoable model from an existing in-memory OpenSim model
        explicit UndoableUiModel(std::unique_ptr<OpenSim::Model> model);

        UiModel const& getUiModel() const;
        UiModel& updUiModel();

        bool canUndo() const noexcept;
        void doUndo();
        bool canRedo() const noexcept;
        void doRedo();

        OpenSim::Model const& getModel() const noexcept;
        OpenSim::Model& updModel() noexcept;
        void setModel(std::unique_ptr<OpenSim::Model>);

        SimTK::State const& getState() const noexcept;
        SimTK::State& updState() noexcept;

        float getFixupScaleFactor() const;
        void setFixupScaleFactor(float);

        float getReccommendedScaleFactor() const;

        void updateIfDirty();

        void setModelDirty(bool);
        void setStateDirty(bool);
        void setDecorationsDirty(bool);
        void setAllDirty(bool);

        bool hasSelected() const;
        OpenSim::Component const* getSelected() const;
        OpenSim::Component* updSelected();
        void setSelected(OpenSim::Component const* c);
        bool selectionHasTypeHashCode(size_t v) const;

        template<typename T>
        bool selectionIsType() const {
            return selectionHasTypeHashCode(typeid(T).hash_code());
        }

        template<typename T>
        bool selectionDerivesFrom() const {
            return getSelectedAs<T>() != nullptr;
        }

        template<typename T>
        T const* getSelectedAs() const {
            return dynamic_cast<T const*>(getSelected());
        }

        template<typename T>
        T* updSelectedAs() {
            return dynamic_cast<T*>(updSelected());
        }

        bool hasHovered() const;
        OpenSim::Component const* getHovered() const noexcept;
        OpenSim::Component* updHovered();
        void setHovered(OpenSim::Component const* c);

        OpenSim::Component const* getIsolated() const noexcept;
        OpenSim::Component* updIsolated();
        void setIsolated(OpenSim::Component const* c);

        // declare the death of a component pointer
        //
        // this happens when we know that OpenSim has destructed a component in
        // the model indirectly (e.g. it was destructed by an OpenSim container)
        // and that we want to ensure the pointer isn't still held by this state
        void declareDeathOf(OpenSim::Component const* c) noexcept;

    private:
        UiModel m_Current;
        UiModel m_Backup;
        CircularBuffer<UiModel, 32> m_UndoBuffer;
        CircularBuffer<UiModel, 32> m_RedoBuffer;
    };
}

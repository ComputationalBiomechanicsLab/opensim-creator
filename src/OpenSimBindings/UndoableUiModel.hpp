#pragma once

#include <memory>

namespace OpenSim {
    class Model;
    class Component;
}

namespace SimTK {
    class State;
}

namespace osc {
    class UiModel;
}

namespace osc {
    // A "UI ready" model with undo/redo support
    class UndoableUiModel final {
    public:
        // construct a new, blank, UndoableUiModel
        UndoableUiModel();

        // construct a new UndoableUiModel from an existing in-memory OpenSim model
        explicit UndoableUiModel(std::unique_ptr<OpenSim::Model> model);

        UndoableUiModel(UndoableUiModel const&) = delete;

        // move an UndoableUiModel in memory
        UndoableUiModel(UndoableUiModel&&) noexcept;

        ~UndoableUiModel() noexcept;

        UndoableUiModel& operator=(UndoableUiModel const&) = delete;

        // mover another UndoableUiModel over this one
        UndoableUiModel& operator=(UndoableUiModel&&) noexcept;

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

        float getFixupScaleFactor() const;
        void setFixupScaleFactor(float);
        float getReccommendedScaleFactor() const;

        void updateIfDirty();

        void setModelDirtyADVANCED(bool);
        void setStateDirtyADVANCED(bool);
        void setDecorationsDirtyADVANCED(bool);
        void setDirty(bool);

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

    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}

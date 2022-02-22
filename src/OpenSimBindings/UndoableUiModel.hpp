#pragma once

#include <filesystem>
#include <memory>

namespace OpenSim
{
    class Model;
    class Component;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    class UiModel;
}

namespace osc
{
    // A "UI ready" model with undo/redo support
    class UndoableUiModel final {
    public:

        // construct a new, blank, UndoableUiModel
        UndoableUiModel();

        // construct a new UndoableUiModel from an existing in-memory OpenSim model
        explicit UndoableUiModel(std::unique_ptr<OpenSim::Model> model);

        // copy-construct a new UndoableUiModel
        UndoableUiModel(UndoableUiModel const&);

        // move an UndoableUiModel in memory
        UndoableUiModel(UndoableUiModel&&) noexcept;

        // destruct an UndoableUiModel
        ~UndoableUiModel() noexcept;

        // copy-assign some other UndoableUiModel over this one
        UndoableUiModel& operator=(UndoableUiModel const&);

        // move-assign some other UndoableUiModel over this one
        UndoableUiModel& operator=(UndoableUiModel&&) noexcept;

        // returns `true` if the model has a known on-disk location
        bool hasFilesystemLocation() const;

        // returns the full filesystem path of the model's on-disk location, if applicable
        //
        // returns an empty path if the model has not been saved to disk
        std::filesystem::path const& getFilesystemPath() const;

        // sets the full filesystem path of the model's on-disk location
        //
        // setting this to an empty path is interpreted as "no on-disk location"
        void setFilesystemPath(std::filesystem::path const&);

        // returns `true` if the current model commit is up to date with its on-disk representation
        //
        // returns `false` if the model has no on-disk location
        bool isUpToDateWithFilesystem() const;

        // manually sets if the current commit as being up to date with disk
        void setUpToDateWithFilesystem();

        // get/update current UiModel
        //
        // note: mutating anything may trigger an undo/redo save if `isDirty` returns `true`
        UiModel const& getUiModel() const;
        UiModel& updUiModel();

        // manipulate undo/redo state
        bool canUndo() const;
        void doUndo();
        bool canRedo() const;
        void doRedo();

        // try to rollback the model to an early-as-possible state
        void rollback();

        // read/manipulate underlying OpenSim::Model
        //
        // note: mutating anything may trigger an automatic undo/redo save if `isDirty` returns `true`
        OpenSim::Model const& getModel() const;
        OpenSim::Model& updModel();
        void setModel(std::unique_ptr<OpenSim::Model>);

        // gets the `SimTK::State` that's valid against the current model
        SimTK::State const& getState() const;

        // read/manipulate fixup scale factor
        //
        // this is the scale factor used to scale visual things in the UI
        float getFixupScaleFactor() const;
        void setFixupScaleFactor(float);
        float getReccommendedScaleFactor() const;

        // update any underlying derived data (SimTK::State, UI decorations, etc.)
        //
        // ideally, called after a mutation is made. Otherwise, derived items (e.g. state) may
        // be out-of-date with the model
        void updateIfDirty();

        // read/manipulate dirty flags
        //
        // these flags are used to decide which parts of the model/state/decorations need to be
        // updated
        void setModelDirtyADVANCED(bool);
        void setStateDirtyADVANCED(bool);
        void setDecorationsDirtyADVANCED(bool);
        void setDirty(bool);

        // read/manipulate current selection (if any)
        bool hasSelected() const;
        OpenSim::Component const* getSelected() const;
        template<typename T>
        T const* getSelectedAs() const
        {
            return dynamic_cast<T const*>(getSelected());
        }
        OpenSim::Component* updSelected();
        template<typename T>
        T* updSelectedAs()
        {
            return dynamic_cast<T*>(updSelected());
        }
        void setSelected(OpenSim::Component const* c);
        bool selectionHasTypeHashCode(size_t v) const;
        template<typename T>
        bool selectionIsType() const
        {
            return selectionHasTypeHashCode(typeid(T).hash_code());
        }
        template<typename T>
        bool selectionDerivesFrom() const
        {
            return getSelectedAs<T>() != nullptr;
        }

        // read/manipulate current hover (if any)
        bool hasHovered() const;
        OpenSim::Component const* getHovered() const;
        OpenSim::Component* updHovered();
        void setHovered(OpenSim::Component const* c);

        // read/manipulate current isolation (the thing that's only being drawn - if any)
        OpenSim::Component const* getIsolated() const;
        OpenSim::Component* updIsolated();
        void setIsolated(OpenSim::Component const* c);

        // declare the death of a component pointer
        //
        // this happens when we know that OpenSim has destructed a component in
        // the model indirectly (e.g. it was destructed by an OpenSim container)
        // and that we want to ensure the pointer isn't still held by this state
        void declareDeathOf(OpenSim::Component const* c);

    public:
        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}

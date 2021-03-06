#pragma once

#include "src/OpenSimBindings/VirtualModelStatePair.hpp"
#include "src/Utils/UID.hpp"

#include <filesystem>
#include <memory>
#include <string_view>

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
    class ModelStateCommit;
}

namespace osc
{
    // a model + state pair that automatically reinitializes (i.e. like `AutoFinalizingModelStatePair`),
    // but it also has support for snapshotting with .commit()
    class UndoableModelStatePair final : public VirtualModelStatePair {
    public:

        // construct a new, blank, UndoableUiModel
        UndoableModelStatePair();

        // construct a new UndoableUiModel from an existing in-memory OpenSim model
        explicit UndoableModelStatePair(std::unique_ptr<OpenSim::Model> model);

        // copy-construct a new UndoableUiModel
        UndoableModelStatePair(UndoableModelStatePair const&);

        // move an UndoableUiModel in memory
        UndoableModelStatePair(UndoableModelStatePair&&) noexcept;

        // copy-assign some other UndoableUiModel over this one
        UndoableModelStatePair& operator=(UndoableModelStatePair const&);

        // move-assign some other UndoableUiModel over this one
        UndoableModelStatePair& operator=(UndoableModelStatePair&&) noexcept;

        // destruct an UndoableUiModel
        ~UndoableModelStatePair() noexcept override;

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

        // manually sets if the current commit as being up to date with disk at the given timepoint
        void setUpToDateWithFilesystem(std::filesystem::file_time_type);

        // gets the last time when the model was set as up to date with the filesystem
        std::filesystem::file_time_type getLastFilesystemWriteTime() const;

        // returns latest *comitted* model state (i.e. not the one being actively edited, but the one saved into
        // the safer undo/redo buffer)
        ModelStateCommit const& getLatestCommit() const;

        // manipulate undo/redo state
        bool canUndo() const;
        void doUndo();
        bool canRedo() const;
        void doRedo();

        // commit current scratch state to storage
        void commit(std::string_view);

        // try to rollback the model to a recent-as-possible state
        void rollback();

        // read/manipulate underlying OpenSim::Model
        //
        // note: mutating anything may trigger an automatic undo/redo save if `isDirty` returns `true`
        OpenSim::Model const& getModel() const override;
        OpenSim::Model& updModel();
        void setModel(std::unique_ptr<OpenSim::Model>);
        UID getModelVersion() const override;
        void setModelVersion(UID);

        // gets the `SimTK::State` that's valid against the current model
        SimTK::State const& getState() const override;
        UID getStateVersion() const override;

        // read/manipulate fixup scale factor
        //
        // this is the scale factor used to scale visual things in the UI
        float getFixupScaleFactor() const override;
        void setFixupScaleFactor(float) override;

        // read/manipulate current selection (if any)
        OpenSim::Component const* getSelected() const override;
        void setSelected(OpenSim::Component const* c) override;

        // read/manipulate current hover (if any)
        OpenSim::Component const* getHovered() const override;
        void setHovered(OpenSim::Component const* c) override;

        // read/manipulate current isolation (the thing that's only being drawn - if any)
        OpenSim::Component const* getIsolated() const override;
        void setIsolated(OpenSim::Component const* c) override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}

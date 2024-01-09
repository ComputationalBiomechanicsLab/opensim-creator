#pragma once

#include <OpenSimCreator/Documents/Model/IModelStatePair.hpp>

#include <oscar/Utils/UID.hpp>

#include <filesystem>
#include <memory>
#include <string_view>

namespace OpenSim { class Model; }
namespace OpenSim { class Component; }
namespace osc { class ModelStateCommit; }
namespace SimTK { class State; }

namespace osc
{
    // a model + state pair that automatically reinitializes (i.e. like `AutoFinalizingModelStatePair`),
    // but it also has support for snapshotting with .commit()
    class UndoableModelStatePair final : public IModelStatePair {
    public:

        // constructs a blank model
        UndoableModelStatePair();

        // constructs a model from an existing in-memory OpenSim model
        explicit UndoableModelStatePair(std::unique_ptr<OpenSim::Model> model);

        // construct a model by loading an existing on-disk osim file
        explicit UndoableModelStatePair(std::filesystem::path const& osimPath);

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
        void commit(std::string_view commitMessage);

        // try to rollback the model to a recent-as-possible state
        void rollback();

        // try to checkout the given commit as the latest commit
        bool tryCheckout(ModelStateCommit const&);

        // read/manipulate underlying OpenSim::Model
        //
        // note: mutating anything may trigger an automatic undo/redo save if `isDirty` returns `true`
        OpenSim::Model& updModel();
        void setModel(std::unique_ptr<OpenSim::Model>);
        void setModelVersion(UID);

    private:
        OpenSim::Model const& implGetModel() const final;
        UID implGetModelVersion() const final;

        SimTK::State const& implGetState() const final;
        UID implGetStateVersion() const final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        OpenSim::Component const* implGetSelected() const final;
        void implSetSelected(OpenSim::Component const* c) final;

        OpenSim::Component const* implGetHovered() const final;
        void implSetHovered(OpenSim::Component const* c) final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

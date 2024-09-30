#pragma once

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>

#include <oscar/Utils/UID.h>

#include <filesystem>
#include <memory>
#include <string_view>

namespace OpenSim { class Model; }
namespace OpenSim { class Component; }
namespace osc { class ModelStateCommit; }
namespace SimTK { class State; }

namespace osc
{
    // `UndoableModelStatePair` is an `IModelStatePair` that's designed for immediate UI usage.
    class UndoableModelStatePair final : public IModelStatePair {
    public:

        // constructs a blank model
        UndoableModelStatePair();

        // constructs a model from an existing in-memory OpenSim model
        explicit UndoableModelStatePair(const OpenSim::Model&);

        // constructs a model from an existing in-memory OpenSim model
        explicit UndoableModelStatePair(std::unique_ptr<OpenSim::Model> model);

        // construct a model by loading an existing on-disk osim file
        explicit UndoableModelStatePair(const std::filesystem::path& osimPath);

        // copy-construct a new UndoableUiModel
        UndoableModelStatePair(const UndoableModelStatePair&);

        // move an UndoableUiModel in memory
        UndoableModelStatePair(UndoableModelStatePair&&) noexcept;

        // copy-assign some other UndoableUiModel over this one
        UndoableModelStatePair& operator=(const UndoableModelStatePair&);

        // move-assign some other UndoableUiModel over this one
        UndoableModelStatePair& operator=(UndoableModelStatePair&&) noexcept;

        // destruct an UndoableUiModel
        ~UndoableModelStatePair() noexcept override;

        // returns `true` if the model has a known on-disk location
        bool hasFilesystemLocation() const;

        // returns a string representation of the recommended document's name
        std::string recommendedDocumentName() const;

        // returns the full filesystem path of the model's on-disk location, if applicable
        //
        // returns an empty path if the model has not been saved to disk
        std::filesystem::path getFilesystemPath() const;

        // sets the full filesystem path of the model's on-disk location
        //
        // setting this to an empty path is interpreted as "no on-disk location"
        void setFilesystemPath(const std::filesystem::path&);

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
        ModelStateCommit getLatestCommit() const;

        // manipulate undo/redo state
        bool canUndo() const;
        void doUndo();
        bool canRedo() const;
        void doRedo();

        // try to rollback the model to a recent-as-possible state
        void rollback();

        // try to checkout the given commit as the latest commit
        bool tryCheckout(const ModelStateCommit&);

        // read/manipulate underlying OpenSim::Model
        void setModel(std::unique_ptr<OpenSim::Model>);
        void resetModel();
        void loadModel(const std::filesystem::path&);

        SimTK::State& updState();

    private:
        const OpenSim::Model& implGetModel() const final;
        const SimTK::State& implGetState() const final;

        bool implCanUpdModel() const final { return true; }
        OpenSim::Model& implUpdModel() final;

        void implCommit(std::string_view) final;

        UID implGetModelVersion() const final;
        void implSetModelVersion(UID) final;
        UID implGetStateVersion() const final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        const OpenSim::Component* implGetSelected() const final;
        void implSetSelected(const OpenSim::Component* c) final;

        const OpenSim::Component* implGetHovered() const final;
        void implSetHovered(const OpenSim::Component* c) final;

        std::shared_ptr<Environment> implUpdAssociatedEnvironment() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

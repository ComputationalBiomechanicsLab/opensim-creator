#pragma once

#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>

#include <liboscar/utilities/uid.h>

#include <filesystem>
#include <memory>
#include <string_view>

namespace OpenSim { class Model; }
namespace OpenSim { class Component; }
namespace osc { class ModelStateCommit; }
namespace SimTK { class State; }

namespace osc
{
    // `UndoableModelStatePair` is an `ModelStatePair` that's designed for immediate UI usage.
    class UndoableModelStatePair final : public ModelStatePairWithSharedEnvironment {
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

        // returns `true` if the current model commit is up to date with its on-disk representation
        //
        // returns `false` if the model has no on-disk location
        bool isUpToDateWithFilesystem() const;

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

    private:
        const OpenSim::Model& impl_get_model() const final;
        const SimTK::State& impl_get_state() const final;

        bool impl_can_upd_model() const final { return true; }
        OpenSim::Model& impl_upd_model() final;

        void impl_commit(std::string_view) final;

        UID impl_get_model_version() const final;
        void impl_set_model_version(UID) final;
        UID impl_get_state_version() const final;

        float impl_get_fixup_scale_factor() const final;
        void impl_set_fixup_scale_factor(float) final;

        const OpenSim::Component* impl_get_selected() const final;
        void impl_set_selected(const OpenSim::Component* c) final;

        const OpenSim::Component* impl_get_hovered() const final;
        void impl_set_hovered(const OpenSim::Component* c) final;

        std::shared_ptr<Environment> implUpdAssociatedEnvironment() const final;

        void impl_set_up_to_date_with_filesystem(std::filesystem::file_time_type) final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

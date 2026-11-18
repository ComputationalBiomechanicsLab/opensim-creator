#pragma once

#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>

#include <liboscar/utilities/uid.h>

#include <filesystem>
#include <memory>
#include <string_view>

namespace OpenSim
{
    class Component;
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    class ModelStateCommit;

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
        explicit UndoableModelStatePair(const std::filesystem::path& osim_path);

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
        bool is_up_to_date_with_filesystem() const;

        // gets the last time when the model was set as up to date with the filesystem
        std::filesystem::file_time_type get_last_filesystem_write_time() const;

        // returns latest *comitted* model state (i.e. not the one being actively edited, but the
        // one saved into the safer undo/redo buffer)
        ModelStateCommit get_latest_commit() const;

        // manipulate undo/redo state
        bool can_undo() const;
        void do_undo();
        bool can_redo() const;
        void do_redo();

        // try to rollback the model to a recent-as-possible state
        void rollback();

        // try to checkout the given commit as the latest commit
        bool try_checkout(const ModelStateCommit&);

        // read/manipulate underlying OpenSim::Model
        void set_model(std::unique_ptr<OpenSim::Model>);
        void reset_model();
        void load_model(const std::filesystem::path&);

    private:
        const OpenSim::Model& impl_get_model() const override;
        const SimTK::State& impl_get_state() const override;

        bool impl_can_upd_model() const override { return true; }
        OpenSim::Model& impl_upd_model() override;

        void impl_commit(std::string_view) override;

        UID impl_get_model_version() const override;
        void impl_set_model_version(UID) override;
        UID impl_get_state_version() const override;

        float impl_get_fixup_scale_factor() const override;
        void impl_set_fixup_scale_factor(float) override;

        const OpenSim::Component* impl_get_selected() const override;
        void impl_set_selected(const OpenSim::Component* c) override;

        const OpenSim::Component* impl_get_hovered() const override;
        void impl_set_hovered(const OpenSim::Component* c) override;

        std::shared_ptr<Environment> impl_upd_associated_environment() const override;

        void impl_set_up_to_date_with_filesystem(std::filesystem::file_time_type) override;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

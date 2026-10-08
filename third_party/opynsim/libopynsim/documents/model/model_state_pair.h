#pragma once

#include <libopynsim/documents/model/versioned_component_accessor.h>
#include <liboscar/utilities/uid.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace SimTK { class State; }

namespace opyn
{
    // virtual accessor to an `OpenSim::Model` + `SimTK::State` pair, with
    // additional opt-in overrides to aid rendering/UX etc.
    class ModelStatePair : public VersionedComponentAccessor {
    protected:
        ModelStatePair() = default;
        ModelStatePair(const ModelStatePair&) = default;
        ModelStatePair(ModelStatePair&&) noexcept = default;
        ModelStatePair& operator=(const ModelStatePair&) = default;
        ModelStatePair& operator=(ModelStatePair&&) noexcept = default;

        friend bool operator==(const ModelStatePair&, const ModelStatePair&) = default;

    public:
        virtual ~ModelStatePair() noexcept = default;

        const OpenSim::Model& get_model() const { return impl_get_model(); }
        operator const OpenSim::Model& () const { return impl_get_model(); }
        const OpenSim::Model* operator->() const { return &impl_get_model(); }

        const SimTK::State& get_state() const { return impl_get_state(); }

        bool is_readonly() const { return not impl_can_upd_model(); }
        bool can_upd_model() const { return impl_can_upd_model(); }
        OpenSim::Model& upd_model() { return impl_upd_model(); }

        // commit current scratch state to storage
        void commit(std::string_view commitMessage) { impl_commit(commitMessage); }

        osc::UID get_model_version() const { return impl_get_model_version(); }
        void set_model_version(osc::UID id) { impl_set_model_version(id); }
        osc::UID get_state_version() const { return impl_get_state_version(); }

        const OpenSim::Component* get_selected() const
        {
            return impl_get_selected();
        }

        template<typename T>
        const T* get_selected_as() const
        {
            return dynamic_cast<const T*>(get_selected());
        }

        void set_selected(const OpenSim::Component* newSelection)
        {
            impl_set_selected(newSelection);
        }

        void clear_selected() { set_selected(nullptr); }

        const OpenSim::Component* get_hovered() const
        {
            return impl_get_hovered();
        }

        void set_hovered(const OpenSim::Component* newHover)
        {
            impl_set_hovered(newHover);
        }

        // used to scale weird models (e.g. fly leg) in the UI
        float get_fixup_scale_factor() const
        {
            return impl_get_fixup_scale_factor();
        }

        void set_fixup_scale_factor(float newScaleFactor)
        {
            impl_set_fixup_scale_factor(newScaleFactor);
        }

        // if supported by the implementation, manually sets if the current model
        // state pair as being up to date with disk at the given timepoint
        void set_up_to_date_with_filesystem(std::filesystem::file_time_type t) { impl_set_up_to_date_with_filesystem(t); }

    private:
        // overrides + specializes `ComponentAccessor` API
        const OpenSim::Component& impl_get_component() const final;
        bool impl_can_upd_component() const { return impl_can_upd_model(); }
        OpenSim::Component& impl_upd_component() final;
        osc::UID impl_get_component_version() const final { return impl_get_model_version(); }
        void impl_set_component_version(osc::UID newVersion) final { impl_set_model_version(newVersion); }

        // Implementors should return a const reference to an initialized (finalized properties, etc.) model.
        virtual const OpenSim::Model& impl_get_model() const = 0;

        // Implementors should return a const reference to a state that's compatible with the model returned by `impl_get_model`.
        virtual const SimTK::State& impl_get_state() const = 0;

        // Implementors may return whether the model contained by the concrete `ModelStatePair` implementation can be
        // modified in-place.
        //
        // If the response can be `true`, implementors should also override `impl_upd_model` accordingly.
        virtual bool impl_can_upd_model() const { return false; }

        // Implementors may return a mutable reference to a model. It is up to the caller of `upd_model` to ensure that
        // the model is still valid + initialized after modification.
        //
        // If this is implemented, implementors should override `impl_can_upd_model` accordingly.
        virtual OpenSim::Model& impl_upd_model()
        {
            throw std::runtime_error{"model updating not implemented for this type of model state pair"};
        }

        // Implementors may "snapshot" or log the current model + state. It is implementation-defined what
        // exactly (if anything) this means.
        virtual void impl_commit(std::string_view) {}

        // Implementors may return a `UID` that uniquely identifies the current state of the model.
        virtual osc::UID impl_get_model_version() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return osc::UID{};
        }

        // Implementors may use this to manually set the version of a model (sometimes useful for caching)
        virtual void impl_set_model_version(osc::UID) {}

        // Implementors may return a  `UID` that uniquely identifies the current state of the state.
        virtual osc::UID impl_get_state_version() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return osc::UID{};
        }

        virtual const OpenSim::Component* impl_get_selected() const { return nullptr; }
        virtual const OpenSim::Component* impl_get_hovered() const { return nullptr; }
        virtual float impl_get_fixup_scale_factor() const { return 1.0f; }
        virtual void impl_set_fixup_scale_factor(float) {}
        virtual void impl_set_selected(const OpenSim::Component*) {}
        virtual void impl_set_hovered(const OpenSim::Component*) {}
        virtual void impl_set_up_to_date_with_filesystem(std::filesystem::file_time_type) {}
    };
}

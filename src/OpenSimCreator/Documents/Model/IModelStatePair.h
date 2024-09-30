#pragma once

#include <oscar/Utils/UID.h>

#include <memory>
#include <stdexcept>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace SimTK { class State; }
namespace osc { class Environment; }

namespace osc
{
    // virtual accessor to an `OpenSim::Model` + `SimTK::State` pair, with
    // additional opt-in overrides to aid rendering/UX etc.
    class IModelStatePair {
    protected:
        IModelStatePair() = default;
        IModelStatePair(const IModelStatePair&) = default;
        IModelStatePair(IModelStatePair&&) noexcept = default;
        IModelStatePair& operator=(const IModelStatePair&) = default;
        IModelStatePair& operator=(IModelStatePair&&) noexcept = default;
    public:
        virtual ~IModelStatePair() noexcept = default;

        const OpenSim::Model& getModel() const { return implGetModel(); }

        // Implicitly converts a `const IModelStatePair&` into a `const OpenSim::Model&`
        operator const OpenSim::Model& () const { return getModel(); }

        const SimTK::State& getState() const { return implGetState(); }

        bool isReadonly() const { return not implCanUpdModel(); }
        bool canUpdModel() const { return implCanUpdModel(); }
        OpenSim::Model& updModel() { return implUpdModel(); }

        // commit current scratch state to storage
        void commit(std::string_view commitMessage) { implCommit(commitMessage); }

        UID getModelVersion() const { return implGetModelVersion(); }
        void setModelVersion(UID id) { implSetModelVersion(id); }
        UID getStateVersion() const { return implGetStateVersion(); }

        const OpenSim::Component* getSelected() const
        {
            return implGetSelected();
        }

        template<typename T>
        const T* getSelectedAs() const
        {
            return dynamic_cast<const T*>(getSelected());
        }

        void clearSelected() { setSelected(nullptr); }

        const OpenSim::Component* getHovered() const
        {
            return implGetHovered();
        }

        // used to scale weird models (e.g. fly leg) in the UI
        float getFixupScaleFactor() const
        {
            return implGetFixupScaleFactor();
        }

        void setSelected(const OpenSim::Component* newSelection)
        {
            implSetSelected(newSelection);
        }

        void setHovered(const OpenSim::Component* newHover)
        {
            implSetHovered(newHover);
        }

        void setFixupScaleFactor(float newScaleFactor)
        {
            implSetFixupScaleFactor(newScaleFactor);
        }

        std::shared_ptr<Environment> tryUpdEnvironment() const { return implUpdAssociatedEnvironment(); }

    private:
        // Implementors should return a const reference to an initialized (finalized properties, etc.) model.
        virtual const OpenSim::Model& implGetModel() const = 0;

        // Implementors should return a const reference to a state that's compatible with the model returned by `implGetModel`.
        virtual const SimTK::State& implGetState() const = 0;

        // Implementors may return whether the model contained by the concrete `IModelStatePair` implementation can be
        // modified in-place.
        //
        // If the response can be `true`, implementors should also override `implUpdModel` accordingly.
        virtual bool implCanUpdModel() const { return false; }

        // Implementors may return a mutable reference to a model. It is up to the caller of `updModel` to ensure that
        // the model is still valid + initialized after modification.
        //
        // If this is implemented, implementors should override `implCanUpdModel` accordingly.
        virtual OpenSim::Model& implUpdModel()
        {
            throw std::runtime_error{"model updating not implemented for this type of model state pair"};
        }

        // Implementors may "snapshot" or log the current model + state. It is implementation-defined what
        // exactly (if anything) this means.
        virtual void implCommit(std::string_view) {}

        // Implementors may return a `UID` that uniquely identifies the current state of the model.
        virtual UID implGetModelVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return UID{};
        }

        // Implementors may use this to manually set the version of a model (sometimes useful for caching)
        virtual void implSetModelVersion(UID) {}

        // Implementors may return a  `UID` that uniquely identifies the current state of the state.
        virtual UID implGetStateVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return UID{};
        }

        virtual const OpenSim::Component* implGetSelected() const { return nullptr; }
        virtual const OpenSim::Component* implGetHovered() const { return nullptr; }
        virtual float implGetFixupScaleFactor() const { return 1.0f; }
        virtual void implSetSelected(const OpenSim::Component*) {}
        virtual void implSetHovered(const OpenSim::Component*) {}
        virtual void implSetFixupScaleFactor(float) {}

        virtual std::shared_ptr<Environment> implUpdAssociatedEnvironment() const { return nullptr; }
    };
}

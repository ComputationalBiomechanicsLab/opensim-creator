#pragma once

#include "src/OpenSimBindings/VirtualModelStatePair.hpp"
#include "src/Utils/ClonePtr.hpp"
#include "src/Utils/UID.hpp"

#include <nonstd/span.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <memory>
#include <string>

namespace OpenSim
{
    class Component;
    class Coordinate;
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    struct CoordinateEdit;
}

namespace osc
{
    // a model + state pair that automatically updates (e.g. finalizeFromProperties etc.)
    // whenever the model is edited, guaranteeing that any getters/updater member functions
    // return an up-to-date model/state
    class UiModel final : public VirtualModelStatePair {
    public:
        // construct a blank (new) UiModel
        UiModel();

        // construct a UiModel by loading an osim file
        explicit UiModel(std::string const& osim);

        // construct a UiModel from an in-memory OpenSim::Model
        explicit UiModel(std::unique_ptr<OpenSim::Model>);

        UiModel(UiModel const&);
        UiModel(UiModel&&) noexcept;
        UiModel& operator=(UiModel const&);
        UiModel& operator=(UiModel&&) noexcept;
        ~UiModel() noexcept override;

        // get underlying `OpenSim::Model` that the UiModel wraps
        OpenSim::Model const& getModel() const override;

        OpenSim::Model& updModel() override;

        // update the model without modifying the version
        OpenSim::Model& peekModelADVANCED();

        // manually modify the version
        void markModelAsModified();
        void setModel(std::unique_ptr<OpenSim::Model>);

        UID getModelVersion() const override;

        // get associated (default + state modifications) model state
        SimTK::State const& getState() const override;
        UID getStateVersion() const override;

        // push a coordinate state modification to the model (dirties state)
        void pushCoordinateEdit(OpenSim::Coordinate const&, CoordinateEdit const&);

        // remove a state modification from the model (dirties state)
        bool removeCoordinateEdit(OpenSim::Coordinate const&);

        // get the fixup scale factor used to generate scene decorations
        float getFixupScaleFactor() const override;

        // set the fixup scale factor used to generate scene decorations (dirties decorations)
        void setFixupScaleFactor(float) override;

        // returns true if the model internally needs to update in order to return a valid model/state
        bool isDirty() const;

        // sets dirty flags (advanced)
        //
        // dirty flags are usually automatically set by the various mutating
        // methods (e.g. `updModel` will dirty the model). However, it's
        // sometimes necessary to manually set the flags. Common scenarios:
        //
        // - downstream code `const_cast`ed from a non-mutating method, that
        //   code should probably set a dirty flag
        //
        // - downstream code knows the extent of a modification. E.g. the code
        //   might use `updModel` to mutate the model but knows that it's not
        //   necessary to call finalizeFromProperties or rebuild the system for
        //   the model, so it un-dirties the model + state and leaves the
        //   decorations marked as dirty
        void setDirty(bool);

        // updates all members in this class to reflect the latest model
        //
        // this potentially can, depending on what's been modified:
        //
        // - finalize the model's properties + connections (if the model is dirty)
        // - make a new SimTK::System (if the model is dirty)
        // - make a new SimTK::State (if the model/state is dirty)
        // - generate new decorations (if the model/state/decorations is dirty)
        // - update the scene BVH (if model/state/decorations is dirty)
        //
        // so this method has A LOT of potential to THROW. Callers should handle that
        // appropriately (e.g. by reversing the change)
        void updateIfDirty();

        OpenSim::Component const* getSelected() const override;
        OpenSim::Component* updSelected() override;
        void setSelected(OpenSim::Component const* c) override;

        OpenSim::Component const* getHovered() const override;
        OpenSim::Component* updHovered() override;
        void setHovered(OpenSim::Component const* c) override;

        OpenSim::Component const* getIsolated() const override;
        OpenSim::Component* updIsolated() override;
        void setIsolated(OpenSim::Component const* c) override;

        class Impl;
    private:
        ClonePtr<Impl> m_Impl;
    };
}

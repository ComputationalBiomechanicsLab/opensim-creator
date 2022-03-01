#pragma once

#include "src/3D/Model.hpp"  // AABB
#include "src/OpenSimBindings/RenderableScene.hpp"
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
    class Joint;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    struct BVH;
    class StateModifications;
    struct CoordinateEdit;
    struct FdParams;
    struct UiSimulation;
}

namespace osc
{
    // a "UI-ready" OpenSim::Model with an associated (rendered) state
    //
    // this class guarantees that the returned model/state/decorations are up-to-date
    // by internally checking dirty flags
    class UiModel final : public RenderableScene {
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
        OpenSim::Model const& getModel() const;
        OpenSim::Model& updModel();
        OpenSim::Model& peekModelADVANCED();  // doesn't modify the version IDs
        void markModelAsModified();  // manually modify the version IDs
        void setModel(std::unique_ptr<OpenSim::Model>);
        UID getModelVersion() const;  // changes when the model may have been modified

        // get associated (default + state modifications) model state
        SimTK::State const& getState() const;
        UID getStateVersion() const;

        // get user-enacted state modifications (e.g. coordinate edits)
        StateModifications const& getStateModifications() const;

        // push a coordinate state modification to the model (dirties state)
        void pushCoordinateEdit(OpenSim::Coordinate const&, CoordinateEdit const&);

        // remove a state modification from the model (dirties state)
        bool removeCoordinateEdit(OpenSim::Coordinate const&);

        // get a list of renderable scene elements that represent the model in its state
        nonstd::span<ComponentDecoration const> getSceneDecorations() const override;

        // get a bounding-volume-hierarchy (BVH) for the model's scene decorations
        BVH const& getSceneBVH() const override;

        // get the fixup scale factor used to generate scene decorations
        float getFixupScaleFactor() const override;

        // set the fixup scale factor used to generate scene decorations (dirties decorations)
        void setFixupScaleFactor(float);

        // returns the axis-aligned bounding box (AABB) of the model decorations
        AABB getSceneAABB() const;

        // returns the 3D worldspace dimensions of the model decorations
        glm::vec3 getSceneDimensions() const;

        // returns the longest worldspace dimension of the model decorations
        float getSceneLongestDimension() const;

        // returns what the implementation thinks is a suitable scale factor, given the decoration's dimensions
        float getRecommendedScaleFactor() const;

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


        // returns `true` if something is selected within the model
        bool hasSelected() const;

        // returns a pointer to the currently-selected component, or `nullptr`
        OpenSim::Component const* getSelected() const override;

        // returns a mutable pointer to the currently-selected component, or `nullptr` (dirties model)
        OpenSim::Component* updSelected();

        // sets the current selection
        void setSelected(OpenSim::Component const* c);

        // returns `true` if the given
        bool selectionHasTypeHashCode(size_t v) const;

        // returns `true` if the model has a selection that is of type `T`
        template<typename T>
        bool selectionIsType() const
        {
            return selectionHasTypeHashCode(typeid(T).hash_code());
        }

        // returns `true` if the model has a selection that is, or derives from, `T`
        template<typename T>
        bool selectionDerivesFrom() const
        {
            return dynamic_cast<T const*>(getSelected()) != nullptr;
        }

        // returns a pointer to the current selection, if any, downcasted as `T` (if possible - otherwise nullptr)
        template<typename T>
        T const* getSelectedAs() const
        {
            return dynamic_cast<T const*>(getSelected());
        }

        // returns a pointer to the current selection, if any, downcasted as `T` (if possible - otherwise nullptr)
        //
        // dirties model
        template<typename T>
        T* updSelectedAs()
        {
            return dynamic_cast<T*>(updSelected());
        }


        // returns `true` if something is hovered within the model (e.g. by a mouse)
        bool hasHovered() const;
        OpenSim::Component const* getHovered() const override;
        OpenSim::Component* updHovered();
        void setHovered(OpenSim::Component const* c);


        OpenSim::Component const* getIsolated() const override;
        OpenSim::Component* updIsolated();
        void setIsolated(OpenSim::Component const* c);


        // sets selected, hovered, and isolated state from some other model
        // (i.e. to transfer those pointers accross)
        void setSelectedHoveredAndIsolatedFrom(UiModel const&);

        class Impl;
    private:
        ClonePtr<Impl> m_Impl;
    };
}

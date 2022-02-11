#pragma once

#include "src/3D/Model.hpp"
#include "src/OpenSimBindings/RenderableScene.hpp"

#include <nonstd/span.hpp>
#include <glm/vec3.hpp>

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

namespace OpenSim {
    class Component;
    class Coordinate;
    class Model;
    class Joint;
}

namespace SimTK {
    class State;
}

namespace osc {
    struct BVH;
    class StateModifications;
    struct CoordinateEdit;
    struct FdParams;
    struct UiSimulation;
}

namespace osc {

    // a "UI-ready" OpenSim::Model with an associated (rendered) state
    class UiModel final : public RenderableScene {
    public:
        // construct a blank (new) UiModel
        UiModel();

        // construct a UiModel by loading an osim file
        explicit UiModel(std::string const& osim);

        // construct a UiModel from an in-memory OpenSim::Model
        explicit UiModel(std::unique_ptr<OpenSim::Model>);

        // construct an independent copy of a UiModel
        UiModel(UiModel const&);

        // move the UiModel somewhere else in memory
        UiModel(UiModel&&) noexcept;

        ~UiModel() noexcept override;

        // copy another UiModel over this one
        UiModel& operator=(UiModel const&);

        // move another UiModel over this one
        UiModel& operator=(UiModel&&) noexcept;

        StateModifications const& getStateModifications() const;

        OpenSim::Model const& getModel() const;
        OpenSim::Model& updModel();
        void setModel(std::unique_ptr<OpenSim::Model>);

        SimTK::State const& getState() const;

        // read dirty flag
        //
        // this is set by the various mutating methods and indicate
        // that part of the UiModel *may* be modified in some way
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
        void setModelDirtyADVANCED(bool);
        void setStateDirtyADVANCED(bool);
        void setDecorationsDirtyADVANCED(bool);
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

        nonstd::span<LabelledSceneElement const> getSceneDecorations() const override;

        BVH const& getSceneBVH() const override;

        float getFixupScaleFactor() const override;

        void setFixupScaleFactor(float);

        bool hasSelected() const;
        OpenSim::Component const* getSelected() const override;
        OpenSim::Component* updSelected();
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
            return dynamic_cast<T const*>(getSelected()) != nullptr;
        }
        template<typename T>
        T const* getSelectedAs() const
        {
            return dynamic_cast<T const*>(getSelected());
        }
        template<typename T>
        T* updSelectedAs()
        {
            return dynamic_cast<T*>(updSelected());
        }

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

        void pushCoordinateEdit(OpenSim::Coordinate const&, CoordinateEdit const&);
        bool removeCoordinateEdit(OpenSim::Coordinate const&);

        AABB getSceneAABB() const;
        glm::vec3 getSceneDimensions() const;
        float getSceneLongestDimension() const;
        float getRecommendedScaleFactor() const;

        std::chrono::system_clock::time_point getLastModifiedTime() const;

        // declare the death of a component pointer
        //
        // this happens when we know that OpenSim has destructed a component in
        // the model indirectly (e.g. it was destructed by an OpenSim container)
        // and that we want to ensure the pointer isn't still held by this state
        void declareDeathOf(OpenSim::Component const* c);

    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}

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
    //
    // this is what most of the components, screen elements, etc. are
    // accessing - usually indirectly (e.g. via a reference to the Model)
    class UiModel final : public RenderableScene {
    public:
        // make a blank (new) UiModel
        UiModel();

        // load a UiModel from an osim file
        explicit UiModel(std::string const& osim);

        // make a UiModel from an in-memory OpenSim::Model
        explicit UiModel(std::unique_ptr<OpenSim::Model>);

        // make an independent copy
        UiModel(UiModel const&);

        // move this model somewhere else in memory
        UiModel(UiModel&&) noexcept;

        ~UiModel() noexcept override;

        UiModel& operator=(UiModel const&);
        UiModel& operator=(UiModel&&);

        StateModifications const& getStateModifications() const;

        OpenSim::Model const& getModel() const;
        OpenSim::Model& updModel();
        void setModel(std::unique_ptr<OpenSim::Model>);

        SimTK::State const& getState() const;
        SimTK::State& updState();

        // returns true if the model has been modified but the state etc.
        // haven't yet been updated to reflect the change
        bool isDirty() const;

        // updates all members in this class to reflect the latest model
        //
        // this potentially can, depending on what's been modified:
        //
        // - make a new SimTK::System (if the model is modified)
        // - make a new SimTK::State
        // - generate new decorations
        // - update the scene BVH
        //
        // so this has A LOT of potential to THROW. You should handle that
        // appropriately.
        void updateIfDirty();

        // manually sets the internal dirty flags for this model
        //
        // this is *usually* automatically handled based on other method calls, but
        // is sometimes necessary if (e.g.) the calling code ends up `const_cast`ing
        // a member of this class for practical reasons
        void setModelDirty(bool);
        void setStateDirty(bool);
        void setDecorationsDirty(bool);
        void setAllDirty(bool);

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
        bool selectionIsType() const {
            return selectionHasTypeHashCode(typeid(T).hash_code());
        }
        template<typename T>
        bool selectionDerivesFrom() const {
            return dynamic_cast<T const*>(getSelected()) != nullptr;
        }
        template<typename T>
        T const* getSelectedAs() const {
            return dynamic_cast<T const*>(getSelected());
        }
        template<typename T>
        T* updSelectedAs() {
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
        void declareDeathOf(OpenSim::Component const* c) noexcept;

    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}

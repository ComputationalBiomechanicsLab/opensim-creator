#pragma once

#include "src/3D/BVH.hpp"
#include "src/OpenSimBindings/RenderableScene.hpp"

#include <nonstd/span.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace OpenSim {
    class Model;
    class Coordinate;
}

namespace SimTK {
    class State;
}

namespace osc {

    void generateDecorations(OpenSim::Model const&,
                             SimTK::State const&,
                             float fixupScaleFactor,
                             std::vector<LabelledSceneElement>&);
    void updateBVH(nonstd::span<LabelledSceneElement const>, BVH&);

    // user-enacted coordinate edit
    //
    // used to modify the default state whenever a new state is generated
    struct CoordinateEdit final {
        double value;
        double speed;
        bool locked;

        bool applyToState(OpenSim::Coordinate const&, SimTK::State&) const;  // returns `true` if it modified the state
    };

    // user-enacted state modifications
    class StateModifications final {
    public:
        void pushCoordinateEdit(OpenSim::Coordinate const&, CoordinateEdit const&);
        bool applyToState(OpenSim::Model const&, SimTK::State&) const;

    private:
        std::unordered_map<std::string, CoordinateEdit> m_CoordEdits;
    };

    // a "UI-ready" OpenSim::Model with an associated (rendered) state
    //
    // this is what most of the components, screen elements, etc. are
    // accessing - usually indirectly (e.g. via a reference to the Model)
    struct UiModel final : public RenderableScene {
        // user-enacted state modifications (e.g. coordinate edits)
        StateModifications stateModifications;

        // the model, finalized from its properties
        std::unique_ptr<OpenSim::Model> model;

        // SimTK::State, in a renderable state (e.g. realized up to a relevant stage)
        std::unique_ptr<SimTK::State> state;

        // decorations, generated from model's display properties etc.
        std::vector<LabelledSceneElement> decorations;

        // scene-level BVH of decoration AABBs
        BVH sceneAABBBVH;

        // fixup scale factor of the model
        //
        // this scales up/down the decorations of the model - used for extremely
        // undersized models (e.g. fly leg)
        float fixupScaleFactor;

        // current selection, if any
        OpenSim::Component* selected;

        // current hover, if any
        OpenSim::Component* hovered;

        // current isolation, if any
        //
        // "isolation" here means that the user is only interested in this
        // particular subcomponent in the model, so visualizers etc. should
        // try to only show that component
        OpenSim::Component* isolated;

        // generic timestamp
        //
        // can indicate creation or latest modification, it's here to roughly
        // track how old/new the instance is
        std::chrono::system_clock::time_point timestamp;

        explicit UiModel(std::string const& osim);
        explicit UiModel(std::unique_ptr<OpenSim::Model>);
        UiModel(UiModel const&);
        UiModel(UiModel const&, std::chrono::system_clock::time_point t);
        UiModel(UiModel&&) noexcept;
        ~UiModel() noexcept override;

        UiModel& operator=(UiModel const&) = delete;
        UiModel& operator=(UiModel&&);

        // this should be called whenever `model` is mutated
        //
        // This method updates the other members to reflect the modified model. It
        // can throw, because the modification may have put the model into an invalid
        // state that can't be used to initialize a new SimTK::MultiBodySystem or
        // SimTK::State
        void onUiModelModified();

        nonstd::span<LabelledSceneElement const> getSceneDecorations() const override {
            return decorations;
        }

        BVH const& getSceneBVH() const override {
            return sceneAABBBVH;
        }

        float getFixupScaleFactor() const override {
            return fixupScaleFactor;
        }

        OpenSim::Component const* getSelected() const override {
            return selected;
        }

        OpenSim::Component const* getHovered() const override {
            return hovered;
        }

        OpenSim::Component const* getIsolated() const override {
            return isolated;
        }

        void pushCoordinateEdit(OpenSim::Coordinate const&, CoordinateEdit const&);
    };
}

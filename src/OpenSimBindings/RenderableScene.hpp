#pragma once

#include "src/3D/BVH.hpp"
#include "src/SimTKBindings/SceneGeneratorNew.hpp"

#include <nonstd/span.hpp>

#include <vector>

namespace OpenSim {
    class Component;
    class Model;
}

namespace SimTK {
    class State;
}

namespace osc {
    struct LabelledSceneElement : public SceneElement {
        OpenSim::Component const* component;

        LabelledSceneElement(SceneElement const& se, OpenSim::Component const* c) :
            SceneElement{se}, component{c} {
        }
    };

    class RenderableScene {
    public:
        virtual ~RenderableScene() noexcept = default;
        virtual nonstd::span<LabelledSceneElement const> getSceneDecorations() const = 0;
        virtual BVH const& getSceneBVH() const = 0;
        virtual float getFixupScaleFactor() const = 0;
        virtual OpenSim::Component const* getSelected() const = 0;
        virtual OpenSim::Component const* getHovered() const = 0;
        virtual OpenSim::Component const* getIsolated() const = 0;
    };

    void generateDecorations(OpenSim::Model const&,
                             SimTK::State const&,
                             float fixupScaleFactor,
                             std::vector<LabelledSceneElement>&);

    void updateBVH(nonstd::span<LabelledSceneElement const>, BVH&);
}

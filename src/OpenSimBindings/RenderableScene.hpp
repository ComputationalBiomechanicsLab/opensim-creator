#pragma once

#include "src/3D/BVH.hpp"
#include "src/SimTKBindings/SceneGeneratorNew.hpp"

#include <nonstd/span.hpp>

namespace OpenSim {
    class Component;
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
}

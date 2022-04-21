#pragma once

#include <nonstd/span.hpp>

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
    struct BVH;
    struct ComponentDecoration;
}

namespace osc
{
    // abstract representation of a renderable UI scene
    class RenderableScene {
    public:
        virtual ~RenderableScene() noexcept = default;

        virtual nonstd::span<ComponentDecoration const> getSceneDecorations() const = 0;
        virtual BVH const& getSceneBVH() const = 0;
        virtual float getFixupScaleFactor() const = 0;
        virtual OpenSim::Component const* getSelected() const = 0;
        virtual OpenSim::Component const* getHovered() const = 0;
        virtual OpenSim::Component const* getIsolated() const = 0;
    };
}

#pragma once

#include <liboscar/graphics/scene/scene_decoration_flags.h>

namespace OpenSim { class Component; }
namespace osc { struct SceneDecoration; }

namespace opyn
{
    // functor class that sets a decoration's flags based on selection logic
    class ComponentSceneDecorationFlagsTagger final {
    public:
        ComponentSceneDecorationFlagsTagger(
            const OpenSim::Component* selected,
            const OpenSim::Component* hovered
        );

        void operator()(const OpenSim::Component&, osc::SceneDecoration&);
    private:
        osc::SceneDecorationFlags computeFlags(const OpenSim::Component&) const;

        const OpenSim::Component* selected_;
        const OpenSim::Component* hovered_;
        const OpenSim::Component* last_component_ = nullptr;
        osc::SceneDecorationFlags last_flags_ = osc::SceneDecorationFlag::Default;
    };
}

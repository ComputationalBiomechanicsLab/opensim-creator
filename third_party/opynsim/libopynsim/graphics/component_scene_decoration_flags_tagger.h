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
            const OpenSim::Component* selected_,
            const OpenSim::Component* hovered_
        );

        void operator()(const OpenSim::Component&, osc::SceneDecoration&);
    private:
        osc::SceneDecorationFlags computeFlags(const OpenSim::Component&) const;

        const OpenSim::Component* m_Selected;
        const OpenSim::Component* m_Hovered;
        const OpenSim::Component* m_LastComponent = nullptr;
        osc::SceneDecorationFlags m_LastFlags = osc::SceneDecorationFlag::Default;
    };
}

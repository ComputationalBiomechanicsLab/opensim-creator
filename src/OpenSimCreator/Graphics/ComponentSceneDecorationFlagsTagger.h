#pragma once

#include <oscar/Graphics/Scene/SceneDecorationFlags.h>

namespace OpenSim { class Component; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    // functor class that sets a decoration's flags based on selection logic
    class ComponentSceneDecorationFlagsTagger final {
    public:
        ComponentSceneDecorationFlagsTagger(
            const OpenSim::Component* selected_,
            const OpenSim::Component* hovered_
        );

        void operator()(const OpenSim::Component&, SceneDecoration&);
    private:
        SceneDecorationFlags computeFlags(const OpenSim::Component&) const;

        const OpenSim::Component* m_Selected;
        const OpenSim::Component* m_Hovered;
        const OpenSim::Component* m_LastComponent = nullptr;
        SceneDecorationFlags m_Flags = SceneDecorationFlags::None;
    };
}

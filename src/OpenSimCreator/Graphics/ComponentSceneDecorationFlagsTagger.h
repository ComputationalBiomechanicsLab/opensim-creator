#pragma once

#include <oscar/Graphics/Scene/SceneDecorationFlags.h>

namespace OpenSim { class Component; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    class ComponentSceneDecorationFlagsTagger final {
    public:
        ComponentSceneDecorationFlagsTagger(
            OpenSim::Component const* selected_,
            OpenSim::Component const* hovered_
        );

        void operator()(OpenSim::Component const&, SceneDecoration&);
    private:
        SceneDecorationFlags computeFlags(OpenSim::Component const&) const;

        OpenSim::Component const* m_Selected;
        OpenSim::Component const* m_Hovered;
        OpenSim::Component const* m_LastComponent = nullptr;
        SceneDecorationFlags m_Flags = SceneDecorationFlags::None;
    };
}

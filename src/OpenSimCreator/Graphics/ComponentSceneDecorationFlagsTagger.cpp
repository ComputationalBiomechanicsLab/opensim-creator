#include "ComponentSceneDecorationFlagsTagger.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Graphics/Scene/SceneDecoration.h>

using namespace osc;

osc::ComponentSceneDecorationFlagsTagger::ComponentSceneDecorationFlagsTagger(
    const OpenSim::Component* selected_,
    const OpenSim::Component* hovered_) :
    m_Selected{selected_},
    m_Hovered{hovered_}
{}

void osc::ComponentSceneDecorationFlagsTagger::operator()(
    const OpenSim::Component& component,
    SceneDecoration& decoration)
{
    if (&component != m_LastComponent)
    {
        m_LastFlags = computeFlags(component);
        m_LastComponent = &component;
    }

    decoration.flags |= m_LastFlags;
}

SceneDecorationFlags osc::ComponentSceneDecorationFlagsTagger::computeFlags(
    const OpenSim::Component& component) const
{
    SceneDecorationFlags rv = SceneDecorationFlag::Default;

    // iterate through this component and all of its owners, because
    // selecting/highlighting a parent implies that this component
    // should also be highlighted
    for (const OpenSim::Component* p = &component; p; p = GetOwner(*p)) {
        if (p == m_Selected) {
            rv |= SceneDecorationFlag::RimHighlight0;
        }
        if (p == m_Hovered) {
            rv |= SceneDecorationFlag::RimHighlight1;
        }
    }

    return rv;
}

#include "ComponentSceneDecorationFlagsTagger.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Graphics/Scene/SceneDecoration.h>

using namespace osc;

osc::ComponentSceneDecorationFlagsTagger::ComponentSceneDecorationFlagsTagger(
    const OpenSim::Component* selected_,
    const OpenSim::Component* hovered_) :
    m_Selected{selected_},
    m_Hovered{hovered_}
{
}

void osc::ComponentSceneDecorationFlagsTagger::operator()(
    const OpenSim::Component& component,
    SceneDecoration& decoration)
{
    if (&component != m_LastComponent)
    {
        m_Flags = computeFlags(component);
        m_LastComponent = &component;
    }

    decoration.flags |= m_Flags;
}

SceneDecorationFlags osc::ComponentSceneDecorationFlagsTagger::computeFlags(
    const OpenSim::Component& component) const
{
    SceneDecorationFlags rv = SceneDecorationFlags::None;

    if (&component == m_Selected)
    {
        rv |= SceneDecorationFlags::IsSelected;
    }

    if (&component == m_Hovered)
    {
        rv |= SceneDecorationFlags::IsHovered;
    }

    for (const OpenSim::Component* p = GetOwner(component); p; p = GetOwner(*p))
    {
        if (p == m_Selected)
        {
            rv |= SceneDecorationFlags::IsChildOfSelected;
        }
        if (p == m_Hovered)
        {
            rv |= SceneDecorationFlags::IsChildOfHovered;
        }
    }

    return rv;
}

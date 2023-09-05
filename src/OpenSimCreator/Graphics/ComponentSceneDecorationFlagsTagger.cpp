#include "ComponentSceneDecorationFlagsTagger.hpp"

#include <oscar/Graphics/SceneDecoration.hpp>

#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

osc::ComponentSceneDecorationFlagsTagger::ComponentSceneDecorationFlagsTagger(
    OpenSim::Component const* selected_,
    OpenSim::Component const* hovered_) :
    m_Selected{selected_},
    m_Hovered{hovered_}
{
}

void osc::ComponentSceneDecorationFlagsTagger::operator()(
    OpenSim::Component const& component,
    SceneDecoration& decoration)
{
    if (&component != m_LastComponent)
    {
        m_Flags = ComputeFlags(component);
        m_LastComponent = &component;
    }

    decoration.flags = m_Flags;
}

osc::SceneDecorationFlags osc::ComponentSceneDecorationFlagsTagger::ComputeFlags(
    OpenSim::Component const& component) const
{
    SceneDecorationFlags rv = SceneDecorationFlags::CastsShadows;

    if (&component == m_Selected)
    {
        rv |= SceneDecorationFlags::IsSelected;
    }

    if (&component == m_Hovered)
    {
        rv |= SceneDecorationFlags::IsHovered;
    }

    for (OpenSim::Component const* p = GetOwner(component); p; p = GetOwner(*p))
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

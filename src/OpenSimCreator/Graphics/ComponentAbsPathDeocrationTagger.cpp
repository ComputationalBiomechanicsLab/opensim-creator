#include "ComponentAbsPathDecorationTagger.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <oscar/Graphics/SceneDecoration.hpp>

void osc::ComponentAbsPathDecorationTagger::operator()(
    OpenSim::Component const& component,
    SceneDecoration& decoration)
{
    if (&component != m_LastComponent)
    {
        GetAbsolutePathString(component, m_ID);
        m_LastComponent = &component;
    }

    decoration.id = m_ID;
}

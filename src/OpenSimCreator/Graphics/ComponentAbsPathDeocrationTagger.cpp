#include "ComponentAbsPathDecorationTagger.hpp"

#include <oscar/Graphics/SceneDecoration.hpp>

#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

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

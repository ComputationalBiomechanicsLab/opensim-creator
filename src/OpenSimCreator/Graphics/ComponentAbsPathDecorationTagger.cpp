#include "ComponentAbsPathDecorationTagger.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Graphics/Scene/SceneDecoration.h>

void osc::ComponentAbsPathDecorationTagger::operator()(
    const OpenSim::Component& component,
    SceneDecoration& decoration)
{
    if (&component != m_LastComponent)
    {
        GetAbsolutePathString(component, m_ID);
        m_LastComponent = &component;
    }

    decoration.id = m_ID;
}

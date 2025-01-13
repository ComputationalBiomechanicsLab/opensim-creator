#include "ComponentAbsPathDecorationTagger.h"

#include <libOpenSimCreator/Utils/OpenSimHelpers.h>

#include <liboscar/Graphics/Scene/SceneDecoration.h>

void osc::ComponentAbsPathDecorationTagger::operator()(
    const OpenSim::Component& component,
    SceneDecoration& decoration)
{
    if (&component != m_LastComponent) {
        m_ID = GetAbsolutePathStringName(component);
        m_LastComponent = &component;
    }

    decoration.id = m_ID;
}

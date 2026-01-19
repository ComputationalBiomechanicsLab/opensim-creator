#include "ComponentAbsPathDecorationTagger.h"

#include <libopynsim/Utils/OpenSimHelpers.h>
#include <liboscar/graphics/scene/scene_decoration.h>

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

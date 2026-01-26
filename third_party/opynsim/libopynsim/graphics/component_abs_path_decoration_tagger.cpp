#include "component_abs_path_decoration_tagger.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_decoration.h>

using namespace opyn;

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

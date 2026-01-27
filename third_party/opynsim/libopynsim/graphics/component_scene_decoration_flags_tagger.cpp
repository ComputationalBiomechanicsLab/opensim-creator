#include "component_scene_decoration_flags_tagger.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_decoration.h>

using namespace opyn;

opyn::ComponentSceneDecorationFlagsTagger::ComponentSceneDecorationFlagsTagger(
    const OpenSim::Component* selected_,
    const OpenSim::Component* hovered_) :
    m_Selected{selected_},
    m_Hovered{hovered_}
{}

void opyn::ComponentSceneDecorationFlagsTagger::operator()(
    const OpenSim::Component& component,
    osc::SceneDecoration& decoration)
{
    if (&component != m_LastComponent)
    {
        m_LastFlags = computeFlags(component);
        m_LastComponent = &component;
    }

    decoration.flags |= m_LastFlags;
}

osc::SceneDecorationFlags opyn::ComponentSceneDecorationFlagsTagger::computeFlags(
    const OpenSim::Component& component) const
{
    osc::SceneDecorationFlags rv = osc::SceneDecorationFlag::Default;

    // iterate through this component and all of its owners, because
    // selecting/highlighting a parent implies that this component
    // should also be highlighted
    for (const OpenSim::Component* p = &component; p; p = GetOwner(*p)) {
        if (p == m_Selected) {
            rv |= osc::SceneDecorationFlag::RimHighlight0;
        }
        if (p == m_Hovered) {
            rv |= osc::SceneDecorationFlag::RimHighlight1;
        }
    }

    return rv;
}

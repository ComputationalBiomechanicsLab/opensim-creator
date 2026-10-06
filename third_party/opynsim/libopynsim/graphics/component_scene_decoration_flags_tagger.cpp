#include "component_scene_decoration_flags_tagger.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_decoration.h>

using namespace opyn;

opyn::ComponentSceneDecorationFlagsTagger::ComponentSceneDecorationFlagsTagger(
    const OpenSim::Component* selected,
    const OpenSim::Component* hovered) :
    selected_{selected},
    hovered_{hovered}
{}

void opyn::ComponentSceneDecorationFlagsTagger::operator()(
    const OpenSim::Component& component,
    osc::SceneDecoration& decoration)
{
    if (&component != last_component_)
    {
        last_flags_ = computeFlags(component);
        last_component_ = &component;
    }

    decoration.flags |= last_flags_;
}

osc::SceneDecorationFlags opyn::ComponentSceneDecorationFlagsTagger::computeFlags(
    const OpenSim::Component& component) const
{
    osc::SceneDecorationFlags rv = osc::SceneDecorationFlag::Default;

    // iterate through this component and all of its owners, because
    // selecting/highlighting a parent implies that this component
    // should also be highlighted
    for (const OpenSim::Component* p = &component; p; p = GetOwner(*p)) {
        if (p == selected_) {
            rv |= osc::SceneDecorationFlag::RimHighlight0;
        }
        if (p == hovered_) {
            rv |= osc::SceneDecorationFlag::RimHighlight1;
        }
    }

    return rv;
}

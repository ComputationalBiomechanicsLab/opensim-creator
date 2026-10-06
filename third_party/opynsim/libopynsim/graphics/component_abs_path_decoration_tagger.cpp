#include "component_abs_path_decoration_tagger.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_decoration.h>

using namespace opyn;

void opyn::ComponentAbsPathDecorationTagger::operator()(
    const OpenSim::Component& component,
    osc::SceneDecoration& decoration)
{
    if (&component != last_component_) {
        id_ = GetAbsolutePathStringName(component);
        last_component_ = &component;
    }

    decoration.id = id_;
}

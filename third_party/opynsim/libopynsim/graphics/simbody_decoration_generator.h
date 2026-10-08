#pragma once

#include <functional>

namespace osc { class SceneCache; }
namespace osc { struct SceneDecoration; }
namespace SimTK { class DecorativeGeometry; }
namespace SimTK { class SimbodyMatterSubsystem; }
namespace SimTK { class State; }

namespace opyn
{
    // generates `SceneDecoration`s for the given `SimTK::DecorativeGeometry`
    // and passes them to the output consumer
    void generate_decorations(
        osc::SceneCache&,
        const SimTK::SimbodyMatterSubsystem&,
        const SimTK::State&,
        const SimTK::DecorativeGeometry&,
        float fixup_scale_factor,
        const std::function<void(osc::SceneDecoration&&)>& out
    );
}

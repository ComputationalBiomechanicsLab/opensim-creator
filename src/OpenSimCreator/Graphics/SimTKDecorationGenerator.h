#pragma once

#include <functional>

namespace osc { class SceneCache; }
namespace osc { struct SceneDecoration; }
namespace SimTK { class DecorativeGeometry; }
namespace SimTK { class SimbodyMatterSubsystem; }
namespace SimTK { class State; }

namespace osc
{
    // generates `SceneDecoration`s for the given `SimTK::DecorativeGeometry`
    // and passes them to the output consumer
    void GenerateDecorations(
        SceneCache&,
        const SimTK::SimbodyMatterSubsystem&,
        const SimTK::State&,
        const SimTK::DecorativeGeometry&,
        float fixupScaleFactor,
        std::function<void(SceneDecoration&&)> const& out
    );
}

#pragma once

#include <functional>

namespace osc { class MeshCache; }
namespace osc { struct SimpleSceneDecoration; }
namespace SimTK { class DecorativeGeometry; }
namespace SimTK { class SimbodyMatterSubsystem; }
namespace SimTK { class State; }

namespace osc
{
    // generates `osc::SimpleSceneDecoration`s for the given `SimTK::DecorativeGeometry`
    // and passes them to the output consumer
    void GenerateDecorations(
        MeshCache&,
        SimTK::SimbodyMatterSubsystem const&,
        SimTK::State const&,
        SimTK::DecorativeGeometry const&,
        float fixupScaleFactor,
        std::function<void(SimpleSceneDecoration&&)> const& out
    );
}
#pragma once

#include <functional>

namespace osc { class MeshCache; }
namespace osc { class SimpleSceneDecoration; }
namespace SimTK { class DecorativeGeometry; }
namespace SimTK { class SimbodyMatterSubsystem; }
namespace SimTK { class State; }

namespace osc
{
    void GenerateDecorations(
        MeshCache&,
        SimTK::SimbodyMatterSubsystem const&,
        SimTK::State const&,
        SimTK::DecorativeGeometry const&,
        float fixupScaleFactor,
        std::function<void(SimpleSceneDecoration&&)> const& out);
}
#pragma once

#include <memory>

namespace osc { class MeshCache; }
namespace osc { class SimTKDecorationConsumer; }
namespace SimTK { class DecorativeGeometry; }
namespace SimTK { class SimbodyMatterSubsystem; }
namespace SimTK { class State; }

namespace osc
{
    // consumes SimTK::DecorativeGeometry and emits appropriate decorations back to
    // the `DecorationConsumer`
    class SimTKRenderer final {
    public:
        SimTKRenderer(
            MeshCache&,
            SimTK::SimbodyMatterSubsystem const& matter,
            SimTK::State const& state,
            float fixupScaleFactor,
            SimTKDecorationConsumer&);
        SimTKRenderer(SimTKRenderer const&) = delete;
        SimTKRenderer(SimTKRenderer&&) noexcept;
        SimTKRenderer& operator=(SimTKRenderer const&) = delete;
        SimTKRenderer& operator=(SimTKRenderer&&) noexcept;
        ~SimTKRenderer() noexcept;

        void operator()(SimTK::DecorativeGeometry const&);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
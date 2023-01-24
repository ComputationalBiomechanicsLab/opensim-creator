#pragma once

#include <glm/vec4.hpp>

#include <memory>

namespace osc { class Mesh; }
namespace osc { class MeshCache; }
namespace osc { struct Transform; }
namespace SimTK { class DecorativeGeometry; }
namespace SimTK { class SimbodyMatterSubsystem; }
namespace SimTK { class State; }

namespace osc
{
    // called with an appropriate (output) decoration whenever the
    // DecorativeGeometryHandler wants to emit geometry
    class DecorationConsumer {
    protected:
        DecorationConsumer() = default;
        DecorationConsumer(DecorationConsumer const&) = default;
        DecorationConsumer(DecorationConsumer&&) noexcept = default;
        DecorationConsumer& operator=(DecorationConsumer const&) = default;
        DecorationConsumer& operator=(DecorationConsumer&&) noexcept = default;
    public:
        virtual ~DecorationConsumer() noexcept = default;
        virtual void operator()(Mesh const&, Transform const&, glm::vec4 const& color) = 0;
    };

    // consumes SimTK::DecorativeGeometry and emits appropriate decorations back to
    // the `DecorationConsumer`
    class SimTKRenderer final {
    public:
        SimTKRenderer(
            MeshCache& meshCache,
            SimTK::SimbodyMatterSubsystem const& matter,
            SimTK::State const& state,
            float fixupScaleFactor,
            DecorationConsumer&);
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
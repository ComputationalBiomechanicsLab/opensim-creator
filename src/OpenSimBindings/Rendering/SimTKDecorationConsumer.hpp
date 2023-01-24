#pragma once

#include <glm/vec4.hpp>

namespace osc { class Mesh; }
namespace osc { struct Transform; }

namespace osc
{
    // called with an appropriate (output) decoration whenever the
    // DecorativeGeometryHandler wants to emit geometry
    class SimTKDecorationConsumer {
    protected:
        SimTKDecorationConsumer() = default;
        SimTKDecorationConsumer(SimTKDecorationConsumer const&) = default;
        SimTKDecorationConsumer(SimTKDecorationConsumer&&) noexcept = default;
        SimTKDecorationConsumer& operator=(SimTKDecorationConsumer const&) = default;
        SimTKDecorationConsumer& operator=(SimTKDecorationConsumer&&) noexcept = default;
    public:
        virtual ~SimTKDecorationConsumer() noexcept = default;
        virtual void operator()(Mesh const&, Transform const&, glm::vec4 const& color) = 0;
    };
}
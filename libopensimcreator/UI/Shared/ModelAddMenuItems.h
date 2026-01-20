#pragma once

#include <liboscar/platform/widget.h>

#include <memory>

namespace OpenSim { class ComponentPath; }
namespace osc { class ModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class ModelAddMenuItems final : public Widget {
    public:
        explicit ModelAddMenuItems(
            Widget* parent,
            std::shared_ptr<ModelStatePair> model
        );

        void setTargetParentComponent(const OpenSim::ComponentPath&);
    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

#pragma once

#include <liboscar/platform/widget.h>

#include <memory>

namespace OpenSim
{
    class ComponentPath;
}

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class Widget;

    class ModelAddMenuItems final : public Widget {
    public:
        explicit ModelAddMenuItems(Widget* parent, std::shared_ptr<opyn::ModelStatePair> model);

        void set_target_parent_component(const OpenSim::ComponentPath&);

    private:
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

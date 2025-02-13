#pragma once

#include <liboscar/Platform/Widget.h>

#include <memory>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class ModelActionsMenuItems final : public Widget {
    public:
        explicit ModelActionsMenuItems(
            Widget* parent,
            std::shared_ptr<IModelStatePair>
        );

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

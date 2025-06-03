#pragma once

#include <liboscar/Platform/Widget.h>

#include <memory>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class ModelAddMenuItems final : public Widget {
    public:
        explicit ModelAddMenuItems(
            Widget* parent,
            std::shared_ptr<IModelStatePair>
        );

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

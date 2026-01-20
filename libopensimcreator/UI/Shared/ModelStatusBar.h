#pragma once

#include <liboscar/platform/widget.h>

#include <memory>

namespace osc { class ModelStatePair; }

namespace osc
{
    class ModelStatusBar final : public Widget {
    public:
        explicit ModelStatusBar(
            Widget* parent,
            std::shared_ptr<ModelStatePair>
        );

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

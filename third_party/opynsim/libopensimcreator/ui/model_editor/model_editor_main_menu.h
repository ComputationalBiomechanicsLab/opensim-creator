#pragma once

#include <liboscar/platform/widget.h>

#include <memory>

namespace osc { class ModelStatePairWithSharedEnvironment; }
namespace osc { class PanelManager; }
namespace osc { class Widget; }

namespace osc
{
    class ModelEditorMainMenu final : public Widget {
    public:
        explicit ModelEditorMainMenu(
            Widget* parent,
            std::shared_ptr<PanelManager>,
            std::shared_ptr<ModelStatePairWithSharedEnvironment>
        );
    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

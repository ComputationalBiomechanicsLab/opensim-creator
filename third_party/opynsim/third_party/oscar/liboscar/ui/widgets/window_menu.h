#pragma once

#include <liboscar/platform/widget.h>

#include <memory>

namespace osc { class PanelManager; }

namespace osc
{
    class WindowMenu final : public Widget {
    public:
        explicit WindowMenu(Widget* parent, std::shared_ptr<PanelManager>);

    private:
        void impl_on_draw() final;
        void draw_content();

        std::shared_ptr<PanelManager> panel_manager_;
    };
}

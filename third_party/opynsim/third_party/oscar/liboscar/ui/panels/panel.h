#pragma once

#include <liboscar/platform/widget.h>
#include <liboscar/ui/oscimgui.h>

#include <memory>
#include <string_view>

namespace osc { class PanelPrivate; }

namespace osc
{
    class Panel : public Widget {
    public:
        explicit Panel(
            Widget* parent = nullptr,
            std::string_view panel_name = "unnamed",
            ui::PanelFlags = {}
        );

        bool is_open() const;
        void open();
        void close();

    protected:
        explicit Panel(std::unique_ptr<PanelPrivate>&&);

    private:
        void impl_on_draw() final;  // `Panel` handles drawing the outer panel decorations etc.
        virtual void impl_before_imgui_begin() {}
        virtual void impl_after_imgui_begin() {}
        virtual void impl_draw_content() {}

        OSC_WIDGET_DATA_GETTERS(PanelPrivate);
    };
}

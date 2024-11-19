#pragma once

#include <oscar/Platform/Widget.h>
#include <oscar/UI/oscimgui.h>

#include <memory>
#include <string_view>

namespace osc { class PanelPrivate; }

namespace osc
{
    class Panel : public Widget {
    public:
        Panel();
        Panel(Widget* parent, std::string_view panel_name, ui::WindowFlags = {});

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

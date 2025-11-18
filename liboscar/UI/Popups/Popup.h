#pragma once

#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>

#include <memory>
#include <optional>
#include <string_view>

namespace osc { class PopupPrivate; }

namespace osc
{
    class Popup : public Widget {
    public:
        explicit Popup(
            Widget* parent,
            std::string_view name,
            Vector2 dimensions = {512.0f, 0.0f},
            ui::PanelFlags = ui::PanelFlag::AlwaysAutoResize
        );

        bool is_open();
        void open();
        void close();
        bool begin_popup();
        void end_popup();

    protected:
        explicit Popup(std::unique_ptr<PopupPrivate>&&);

        bool is_popup_opened_this_frame() const;
        void request_close();
        bool is_modal() const;
        void set_modal(bool);
        void set_rect(const Rect&);
        void set_dimensions(Vector2);
        void set_position(std::optional<Vector2>);

        OSC_WIDGET_DATA_GETTERS(PopupPrivate);
    private:
        void impl_on_draw() override;

        // Implementors can/must provide these.
        virtual void impl_before_imgui_begin_popup() {}
        virtual void impl_after_imgui_begin_popup() {}
        virtual void impl_draw_content() = 0;
        virtual void impl_on_close() {}
    };
}

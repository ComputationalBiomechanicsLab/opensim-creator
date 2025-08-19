#pragma once

#include <liboscar/Maths/Vector2.h>
#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/UI/Popups/Popup.h>
#include <liboscar/UI/oscimgui.h>

#include <optional>
#include <string_view>

namespace osc { class Widget; }

namespace osc
{
    class PopupPrivate : public WidgetPrivate {
    public:
        explicit PopupPrivate(
            Popup& owner,
            Widget* parent,
            std::string_view name,
            Vector2 dimensions = {512.0f, 0.0f},
            ui::PanelFlags = ui::PanelFlag::AlwaysAutoResize
        );

    protected:
        bool is_popup_opened_this_frame() const;
        void request_close();
        void set_modal(bool);

        OSC_OWNER_GETTERS(Popup);
    private:
        friend class Popup;

        Vector2i dimensions_;
        std::optional<Vector2i> maybe_position_;
        ui::PanelFlags panel_flags_;
        bool should_open_;
        bool should_close_;
        bool just_opened_;
        bool is_open_;
        bool is_modal_;
    };
}

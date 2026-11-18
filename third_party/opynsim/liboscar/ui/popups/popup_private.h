#pragma once

#include <liboscar/maths/vector.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/oscimgui.h>

#include <optional>
#include <string_view>

namespace osc
{
    class Widget;

    class PopupPrivate : public WidgetPrivate {
    public:
        explicit PopupPrivate(
            Popup& owner,
            Widget* parent,
            std::string_view name,
            Vector2 dimensions = {512.0f, 0.0f},
            ui::PanelFlags = ui::PanelFlag::AlwaysAutoResize);

    protected:
        bool is_popup_opened_this_frame() const;
        void request_close();
        void set_modal(bool);

        OSC_OWNER_GETTERS(Popup);

    private:
        friend class Popup;

        Vector2i dimensions_;
        std::optional<Vector2i> maybe_position_;
        ui::PanelFlags panel_flags_{ui::PanelFlag::None};
        bool should_open_ = false;
        bool should_close_ = false;
        bool just_opened_ = false;
        bool is_open_ = false;
        bool is_modal_ = true;
    };
}

#pragma once

#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/UI/Panels/Panel.h>
#include <liboscar/UI/oscimgui.h>

#include <string>
#include <string_view>

namespace osc
{
    class PanelPrivate : public WidgetPrivate {
    public:
        explicit PanelPrivate(
            Panel& owner,
            Widget* parent = nullptr,
            std::string_view panel_name = "unnamed",
            ui::PanelFlags panel_flags = {}
        );

        bool is_open() const;
        void set_open(bool);

        ui::PanelFlags panel_flags() const { return panel_flags_; }

    protected:
        OSC_OWNER_GETTERS(Panel);
    private:
        std::string panel_enabled_config_key_;
        ui::PanelFlags panel_flags_;
    };
}

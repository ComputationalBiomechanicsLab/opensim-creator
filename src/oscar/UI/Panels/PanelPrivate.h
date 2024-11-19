#pragma once

#include <oscar/Platform/WidgetPrivate.h>
#include <oscar/UI/Panels/Panel.h>
#include <oscar/UI/oscimgui.h>

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
            ui::WindowFlags window_flags = {}
        );

        bool is_open() const;
        void set_open(bool);

        ui::WindowFlags panel_flags() const { return panel_flags_; }
    private:
        std::string panel_enabled_config_key_;
        ui::WindowFlags panel_flags_;
    };
}

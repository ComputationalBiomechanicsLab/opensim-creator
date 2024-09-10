#pragma once

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <string>
#include <string_view>

namespace osc
{
    // a "standard" implementation for an IPanel
    class StandardPanelImpl : public IPanel {
    protected:
        explicit StandardPanelImpl(std::string_view panel_name);
        StandardPanelImpl(std::string_view panel_name, ui::WindowFlags);
        StandardPanelImpl(const StandardPanelImpl&) = default;
        StandardPanelImpl(StandardPanelImpl&&) noexcept = default;
        StandardPanelImpl& operator=(const StandardPanelImpl&) = default;
        StandardPanelImpl& operator=(StandardPanelImpl&&) noexcept = default;
    public:
        ~StandardPanelImpl() noexcept override = default;

    protected:
        void request_close();

    private:
        // this standard implementation supplies these
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        // inheritors can/must provide these
        virtual void impl_before_imgui_begin() {}
        virtual void impl_after_imgui_begin() {}
        virtual void impl_draw_content() = 0;

        std::string panel_name_;
        std::string panel_enabled_config_key_;
        ui::WindowFlags panel_flags_;
    };
}

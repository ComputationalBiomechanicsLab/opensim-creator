#pragma once

#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Panels/Panel.h>

namespace osc
{
    // generic base class for the panels shown in the TPS3D tab
    class MeshWarpingTabPanel : public Panel {
    public:
        using Panel::Panel;

    private:
        void impl_before_imgui_begin() final
        {
            ui::push_style_var(ui::StyleVar::PanelPadding, {0.0f, 0.0f});
        }

        void impl_after_imgui_begin() final
        {
            ui::pop_style_var();
        }
    };
}

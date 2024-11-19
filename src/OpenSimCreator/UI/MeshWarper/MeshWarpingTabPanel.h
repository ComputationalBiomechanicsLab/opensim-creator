#pragma once

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/Panel.h>

namespace osc
{
    // generic base class for the panels shown in the TPS3D tab
    class MeshWarpingTabPanel : public Panel {
    public:
        using Panel::Panel;

    private:
        void impl_before_imgui_begin() final
        {
            ui::push_style_var(ui::StyleVar::WindowPadding, {0.0f, 0.0f});
        }

        void impl_after_imgui_begin() final
        {
            ui::pop_style_var();
        }
    };
}

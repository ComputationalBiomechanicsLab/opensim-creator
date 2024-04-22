#pragma once

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>

namespace osc
{
    // generic base class for the panels shown in the TPS3D tab
    class MeshWarpingTabPanel : public StandardPanelImpl {
    public:
        using StandardPanelImpl::StandardPanelImpl;

    private:
        void impl_before_imgui_begin() final
        {
            ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        }
        void impl_after_imgui_begin() final
        {
            ui::PopStyleVar();
        }
    };
}

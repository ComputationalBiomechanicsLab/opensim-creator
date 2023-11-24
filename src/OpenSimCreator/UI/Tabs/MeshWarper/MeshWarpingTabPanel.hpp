#pragma once

#include <imgui.h>
#include <oscar/UI/Panels/StandardPanel.hpp>

namespace osc
{
    // generic base class for the panels shown in the TPS3D tab
    class MeshWarpingTabPanel : public StandardPanel {
    public:
        using StandardPanel::StandardPanel;

    private:
        void implBeforeImGuiBegin() final
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        }
        void implAfterImGuiBegin() final
        {
            ImGui::PopStyleVar();
        }
    };
}

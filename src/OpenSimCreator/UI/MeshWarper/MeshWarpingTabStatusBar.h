#pragma once

#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    // widget: bottom status bar (shows status messages, hover information, etc.)
    class MeshWarpingTabStatusBar final {
    public:
        MeshWarpingTabStatusBar(
            std::string_view label,
            std::shared_ptr<MeshWarpingTabSharedState const> tabState_) :

            m_Label{label},
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (ui::BeginMainViewportBottomBar(m_Label))
            {
                drawContent();
            }
            ui::End();
        }

    private:
        void drawContent()
        {
            if (m_State->currentHover)
            {
                drawCurrentHoverInfo(*m_State->currentHover);
            }
            else
            {
                ui::TextDisabled("(nothing hovered)");
            }
        }

        void drawCurrentHoverInfo(MeshWarpingTabHover const& hover)
        {
            drawColorCodedXYZ(hover.getWorldspaceLocation());
            ui::SameLine();
            if (hover.isHoveringASceneElement())
            {
                ui::TextDisabled("(Click: select %s)", FindElementNameOr(m_State->getScratch(), *hover.getSceneElementID()).c_str());
            }
            else if (hover.getInput() == TPSDocumentInputIdentifier::Source)
            {
                ui::TextDisabled("(Click: add a landmark, Ctrl+Click: add non-participating landmark)");
            }
            else
            {
                static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
                ui::TextDisabled("(Click: add a landmark)");
            }
        }

        void drawColorCodedXYZ(Vec3 pos)
        {
            ui::TextUnformatted("(");
            ui::SameLine();
            for (int i = 0; i < 3; ++i)
            {
                Color color = {0.5f, 0.5f, 0.5f, 1.0f};
                color[i] = 1.0f;

                ui::PushStyleColor(ImGuiCol_Text, color);
                ui::Text("%f", pos[i]);
                ui::SameLine();
                ui::PopStyleColor();
            }
            ui::TextUnformatted(")");
        }

        std::string m_Label;
        std::shared_ptr<MeshWarpingTabSharedState const> m_State;
    };
}

#pragma once

#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/UI/oscimgui.h>

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
            std::shared_ptr<const MeshWarpingTabSharedState> tabState_) :

            m_Label{label},
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (ui::begin_main_window_bottom_bar(m_Label))
            {
                drawContent();
            }
            ui::end_panel();
        }

    private:
        void drawContent()
        {
            if (m_State->isHoveringSomething()) {
                drawCurrentHoverInfo(m_State->getCurrentHover());
            }
            else {
                ui::draw_text_disabled("(nothing hovered)");
            }
        }

        void drawCurrentHoverInfo(const MeshWarpingTabHover& hover)
        {
            drawColorCodedXYZ(hover.getWorldSpaceLocation());
            ui::same_line();
            if (hover.isHoveringASceneElement())
            {
                ui::draw_text_disabled("(Click: select %s)", FindElementNameOr(m_State->getScratch(), *hover.getSceneElementID()).c_str());
            }
            else if (hover.getInput() == TPSDocumentInputIdentifier::Source)
            {
                ui::draw_text_disabled("(Click: add a landmark, Ctrl+Click: add non-participating landmark)");
            }
            else
            {
                static_assert(num_options<TPSDocumentInputIdentifier>() == 2);
                ui::draw_text_disabled("(Click: add a landmark)");
            }
        }

        void drawColorCodedXYZ(Vec3 pos)
        {
            ui::draw_text("(");
            ui::same_line();
            for (int i = 0; i < 3; ++i) {
                Color color = {0.5f, 0.5f, 0.5f, 1.0f};
                color[i] = 1.0f;

                ui::push_style_color(ui::ColorVar::Text, color);
                ui::draw_text("%f", pos[i]);
                ui::same_line();
                ui::pop_style_color();
            }
            ui::draw_text(")");
        }

        std::string m_Label;
        std::shared_ptr<const MeshWarpingTabSharedState> m_State;
    };
}

#include "ErrorTab.h"

#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/UI/Widgets/LogViewer.h>

#include <IconsFontAwesome5.h>

#include <exception>
#include <memory>
#include <string>

using namespace osc;

class osc::ErrorTab::Impl final : public StandardTabImpl {
public:
    explicit Impl(std::exception const& ex) :
        StandardTabImpl{ICON_FA_SPIDER " Error"},
        m_ErrorMessage{ex.what()}
    {
    }

private:
    void implOnDraw() final
    {
        constexpr float width = 800.0f;
        constexpr float padding = 10.0f;

        Rect tabRect = GetMainViewportWorkspaceScreenRect();
        Vec2 tabDims = dimensions(tabRect);

        // error message panel
        {
            Vec2 pos{tabRect.p1.x + tabDims.x/2.0f, tabRect.p1.y + padding};
            ImGui::SetNextWindowPos(pos, ImGuiCond_Once, {0.5f, 0.0f});
            ImGui::SetNextWindowSize({width, 0.0f});

            if (ImGui::Begin("fatal error"))
            {
                ui::TextWrapped("The application threw an exception with the following message:");
                ImGui::Dummy({2.0f, 10.0f});
                ui::SameLine();
                ui::TextWrapped(m_ErrorMessage);
                ImGui::Dummy({0.0f, 10.0f});
            }
            ImGui::End();
        }

        // log message panel
        {
            Vec2 pos{tabRect.p1.x + tabDims.x/2.0f, tabRect.p2.y - padding};
            ImGui::SetNextWindowPos(pos, ImGuiCond_Once, ImVec2(0.5f, 1.0f));
            ImGui::SetNextWindowSize(ImVec2(width, 0.0f));

            if (ImGui::Begin("Error Log", nullptr, ImGuiWindowFlags_MenuBar))
            {
                m_LogViewer.onDraw();
            }
            ImGui::End();
        }
    }

    std::string m_ErrorMessage;
    LogViewer m_LogViewer;
};


// public API

osc::ErrorTab::ErrorTab(ParentPtr<ITabHost> const&, std::exception const& ex) :
    m_Impl{std::make_unique<Impl>(ex)}
{
}

osc::ErrorTab::ErrorTab(ErrorTab&&) noexcept = default;
osc::ErrorTab& osc::ErrorTab::operator=(ErrorTab&&) noexcept = default;
osc::ErrorTab::~ErrorTab() noexcept = default;

UID osc::ErrorTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::ErrorTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ErrorTab::implOnDraw()
{
    m_Impl->onDraw();
}

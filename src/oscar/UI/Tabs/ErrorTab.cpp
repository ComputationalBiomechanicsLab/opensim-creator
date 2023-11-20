#include "ErrorTab.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/UI/Widgets/LogViewer.hpp>

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <string>
#include <utility>

class osc::ErrorTab::Impl final : public osc::StandardTabBase {
public:
    explicit Impl(std::exception const& ex) :
        StandardTabBase{ICON_FA_SPIDER " Error"},
        m_ErrorMessage{ex.what()}
    {
    }

private:
    void implOnDraw() final
    {
        constexpr float width = 800.0f;
        constexpr float padding = 10.0f;

        Rect tabRect = GetMainViewportWorkspaceScreenRect();
        Vec2 tabDims = Dimensions(tabRect);

        // error message panel
        {
            Vec2 pos{tabRect.p1.x + tabDims.x/2.0f, tabRect.p1.y + padding};
            ImGui::SetNextWindowPos(pos, ImGuiCond_Once, {0.5f, 0.0f});
            ImGui::SetNextWindowSize({width, 0.0f});

            if (ImGui::Begin("fatal error"))
            {
                ImGui::TextWrapped("The application threw an exception with the following message:");
                ImGui::Dummy({2.0f, 10.0f});
                ImGui::SameLine();
                ImGui::TextWrapped("%s", m_ErrorMessage.c_str());
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

osc::ErrorTab::ErrorTab(ParentPtr<TabHost> const&, std::exception const& ex) :
    m_Impl{std::make_unique<Impl>(ex)}
{
}

osc::ErrorTab::ErrorTab(ErrorTab&&) noexcept = default;
osc::ErrorTab& osc::ErrorTab::operator=(ErrorTab&&) noexcept = default;
osc::ErrorTab::~ErrorTab() noexcept = default;

osc::UID osc::ErrorTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ErrorTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ErrorTab::implOnDraw()
{
    m_Impl->onDraw();
}

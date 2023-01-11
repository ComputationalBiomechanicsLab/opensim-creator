#include "ModelEditorToolbar.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::ModelEditorToolbar::Impl final {
public:
    Impl(
        std::string_view label,
        osc::MainUIStateAPI* api,
        std::shared_ptr<osc::UndoableModelStatePair> model) :

        m_Label{std::move(label)},
        m_Parent{api},
        m_Model{std::move(model)}
    {
    }

    void draw()
    {
        float const height = ImGui::GetFrameHeight() + 2.0f*ImGui::GetStyle().WindowPadding.y;
        ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        if (osc::BeginMainViewportTopBar(m_Label, height, flags))
        {
            drawContent();
        }
        ImGui::End();
    }
private:
    void drawContent()
    {
        if (ImGui::Button(ICON_FA_FILE))
        {
        }
    }

    std::string m_Label;
    osc::MainUIStateAPI* m_Parent;
    std::shared_ptr<osc::UndoableModelStatePair> m_Model;
};

osc::ModelEditorToolbar::ModelEditorToolbar(
    std::string_view label,
    MainUIStateAPI* api,
    std::shared_ptr<UndoableModelStatePair> model) :

    m_Impl{std::make_unique<Impl>(std::move(label), api, std::move(model))}
{
}
osc::ModelEditorToolbar::ModelEditorToolbar(ModelEditorToolbar&&) noexcept = default;
osc::ModelEditorToolbar& osc::ModelEditorToolbar::operator=(ModelEditorToolbar&&) noexcept = default;
osc::ModelEditorToolbar::~ModelEditorToolbar() noexcept = default;

void osc::ModelEditorToolbar::draw()
{
    m_Impl->draw();
}
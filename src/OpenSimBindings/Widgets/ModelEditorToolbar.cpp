#include "ModelEditorToolbar.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/Widgets/ParamBlockEditorPopup.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Styling.hpp"

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
        MainUIStateAPI* mainUIStateAPI,
        EditorAPI* editorAPI,
        std::shared_ptr<osc::UndoableModelStatePair> model) :

        m_Label{std::move(label)},
        m_MainUIStateAPI{mainUIStateAPI},
        m_EditorAPI{editorAPI},
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
        ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
        if (ImGui::Button(ICON_FA_PLAY " Simulate (Ctrl+R)"))
        {
            osc::ActionStartSimulatingModel(*m_MainUIStateAPI, *m_Model);
        }
        App::upd().addFrameAnnotation("Simulate Button", osc::GetItemRect());
        ImGui::PopStyleColor();

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_EDIT " Edit simulation settings"))
        {
            m_EditorAPI->pushPopup(std::make_unique<ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI->updSimulationParams()));
        }
    }

    std::string m_Label;
    MainUIStateAPI* m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<osc::UndoableModelStatePair> m_Model;
};

osc::ModelEditorToolbar::ModelEditorToolbar(
    std::string_view label,
    MainUIStateAPI* mainUIStateAPI,
    EditorAPI* editorAPI,
    std::shared_ptr<UndoableModelStatePair> model) :

    m_Impl{std::make_unique<Impl>(std::move(label), mainUIStateAPI, editorAPI, std::move(model))}
{
}
osc::ModelEditorToolbar::ModelEditorToolbar(ModelEditorToolbar&&) noexcept = default;
osc::ModelEditorToolbar& osc::ModelEditorToolbar::operator=(ModelEditorToolbar&&) noexcept = default;
osc::ModelEditorToolbar::~ModelEditorToolbar() noexcept = default;

void osc::ModelEditorToolbar::draw()
{
    m_Impl->draw();
}
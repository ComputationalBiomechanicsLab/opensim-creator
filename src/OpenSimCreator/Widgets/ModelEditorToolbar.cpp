#include "ModelEditorToolbar.hpp"

#include "OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp"
#include "OpenSimCreator/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Widgets/BasicWidgets.hpp"
#include "OpenSimCreator/Widgets/ParamBlockEditorPopup.hpp"
#include "OpenSimCreator/Utils/UndoableModelActions.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/IconCache.hpp>
#include <oscar/Platform/App.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <IconsFontAwesome5.h>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class osc::ModelEditorToolbar::Impl final {
public:
    Impl(
        std::string_view label_,
        std::weak_ptr<MainUIStateAPI> mainUIStateAPI_,
        EditorAPI* editorAPI_,
        std::shared_ptr<osc::UndoableModelStatePair> model_) :

        m_Label{label_},
        m_MainUIStateAPI{std::move(mainUIStateAPI_)},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)}
    {
    }

    void onDraw()
    {
        if (BeginToolbar(m_Label, glm::vec2{5.0f, 5.0f}))
        {
            drawContent();
        }
        ImGui::End();
    }
private:
    void drawModelFileRelatedButtons()
    {
        DrawNewModelButton(m_MainUIStateAPI);
        ImGui::SameLine();
        DrawOpenModelButtonWithRecentFilesDropdown(m_MainUIStateAPI);
        ImGui::SameLine();
        DrawSaveModelButton(m_MainUIStateAPI, *m_Model);
        ImGui::SameLine();
        DrawReloadModelButton(*m_Model);
    }

    void drawForwardDynamicSimulationControls()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2.0f, 0.0f});

        osc::PushStyleColor(ImGuiCol_Text, Color::darkGreen());
        if (ImGui::Button(ICON_FA_PLAY))
        {
            osc::ActionStartSimulatingModel(m_MainUIStateAPI, *m_Model);
        }
        osc::PopStyleColor();
        App::upd().addFrameAnnotation("Simulate Button", osc::GetItemRect());
        osc::DrawTooltipIfItemHovered("Simulate Model", "Run a forward-dynamic simulation of the model");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_EDIT))
        {
            m_EditorAPI->pushPopup(std::make_unique<ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI.lock()->updSimulationParams()));
        }
        osc::DrawTooltipIfItemHovered("Edit Simulation Settings", "Change the parameters used when simulating the model");

        ImGui::PopStyleVar();
    }

    void drawContent()
    {
        drawModelFileRelatedButtons();
        SameLineWithVerticalSeperator();

        DrawUndoAndRedoButtons(*m_Model);
        SameLineWithVerticalSeperator();

        DrawSceneScaleFactorEditorControls(*m_Model);
        SameLineWithVerticalSeperator();

        drawForwardDynamicSimulationControls();
        SameLineWithVerticalSeperator();

        DrawAllDecorationToggleButtons(*m_Model, *m_IconCache);
    }

    std::string m_Label;
    std::weak_ptr<MainUIStateAPI> m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<osc::UndoableModelStatePair> m_Model;

    std::shared_ptr<IconCache> m_IconCache = App::singleton<osc::IconCache>(
        App::resource("icons/"),
        ImGui::GetTextLineHeight()/128.0f
    );
};


// public API

osc::ModelEditorToolbar::ModelEditorToolbar(
    std::string_view label_,
    std::weak_ptr<MainUIStateAPI> mainUIStateAPI_,
    EditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(label_, std::move(mainUIStateAPI_), editorAPI_, std::move(model_))}
{
}
osc::ModelEditorToolbar::ModelEditorToolbar(ModelEditorToolbar&&) noexcept = default;
osc::ModelEditorToolbar& osc::ModelEditorToolbar::operator=(ModelEditorToolbar&&) noexcept = default;
osc::ModelEditorToolbar::~ModelEditorToolbar() noexcept = default;

void osc::ModelEditorToolbar::onDraw()
{
    m_Impl->onDraw();
}

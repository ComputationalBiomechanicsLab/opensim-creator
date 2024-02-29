#include "ModelEditorToolbar.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/ParamBlockEditorPopup.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/IconCache.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/ParentPtr.h>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::ModelEditorToolbar::Impl final {
public:
    Impl(
        std::string_view label_,
        ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
        IEditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> model_) :

        m_Label{label_},
        m_MainUIStateAPI{mainUIStateAPI_},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)}
    {
    }

    void onDraw()
    {
        if (BeginToolbar(m_Label, Vec2{5.0f, 5.0f}))
        {
            drawContent();
        }
        ui::End();
    }
private:
    void drawModelFileRelatedButtons()
    {
        DrawNewModelButton(m_MainUIStateAPI);
        ui::SameLine();
        DrawOpenModelButtonWithRecentFilesDropdown(m_MainUIStateAPI);
        ui::SameLine();
        DrawSaveModelButton(m_MainUIStateAPI, *m_Model);
        ui::SameLine();
        DrawReloadModelButton(*m_Model);
    }

    void drawForwardDynamicSimulationControls()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2.0f, 0.0f});

        PushStyleColor(ImGuiCol_Text, Color::darkGreen());
        if (ui::Button(ICON_FA_PLAY))
        {
            ActionStartSimulatingModel(m_MainUIStateAPI, *m_Model);
        }
        PopStyleColor();
        App::upd().addFrameAnnotation("Simulate Button", GetItemRect());
        DrawTooltipIfItemHovered("Simulate Model", "Run a forward-dynamic simulation of the model");

        ui::SameLine();

        if (ui::Button(ICON_FA_EDIT))
        {
            m_EditorAPI->pushPopup(std::make_unique<ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI->updSimulationParams()));
        }
        DrawTooltipIfItemHovered("Edit Simulation Settings", "Change the parameters used when simulating the model");

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
    ParentPtr<IMainUIStateAPI> m_MainUIStateAPI;
    IEditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;

    std::shared_ptr<IconCache> m_IconCache = App::singleton<IconCache>(
        App::resource_loader().withPrefix("icons/"),
        ImGui::GetTextLineHeight()/128.0f
    );
};


// public API

osc::ModelEditorToolbar::ModelEditorToolbar(
    std::string_view label_,
    ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
    IEditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(label_, mainUIStateAPI_, editorAPI_, std::move(model_))}
{
}
osc::ModelEditorToolbar::ModelEditorToolbar(ModelEditorToolbar&&) noexcept = default;
osc::ModelEditorToolbar& osc::ModelEditorToolbar::operator=(ModelEditorToolbar&&) noexcept = default;
osc::ModelEditorToolbar::~ModelEditorToolbar() noexcept = default;

void osc::ModelEditorToolbar::onDraw()
{
    m_Impl->onDraw();
}

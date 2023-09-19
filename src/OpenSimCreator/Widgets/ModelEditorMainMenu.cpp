#include "ModelEditorMainMenu.hpp"

#include "OpenSimCreator/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp"
#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Panels/ModelMusclePlotPanel.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Utils/UndoableModelActions.hpp"
#include "OpenSimCreator/Widgets/ExportPointsPopup.hpp"
#include "OpenSimCreator/Widgets/MainMenu.hpp"
#include "OpenSimCreator/Widgets/ModelActionsMenuItems.hpp"
#include "OpenSimCreator/Widgets/ParamBlockEditorPopup.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/UI/Widgets/WindowMenu.hpp>

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <memory>
#include <utility>

class osc::ModelEditorMainMenu::Impl final {
public:
    Impl(
        ParentPtr<MainUIStateAPI> const& mainStateAPI_,
        EditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> model_) :

        m_MainUIStateAPI{mainStateAPI_},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)}
    {
    }

    void onDraw()
    {
        m_MainMenuFileTab.onDraw(m_MainUIStateAPI, m_Model.get());
        drawMainMenuEditTab();
        drawMainMenuAddTab();
        drawMainMenuToolsTab();
        drawMainMenuActionsTab();
        m_WindowMenu.onDraw();
        m_MainMenuAboutTab.onDraw();
    }

private:
    void drawMainMenuEditTab()
    {
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, m_Model->canUndo()))
            {
                osc::ActionUndoCurrentlyEditedModel(*m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, m_Model->canRedo()))
            {
                osc::ActionRedoCurrentlyEditedModel(*m_Model);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("         Deselect", nullptr, false, m_Model->getSelected() != nullptr))
            {
                m_Model->setSelected(nullptr);
            }

            ImGui::EndMenu();
        }
    }

    void drawMainMenuAddTab()
    {
        if (ImGui::BeginMenu("Add"))
        {
            m_MainMenuAddTabMenuItems.onDraw();
            ImGui::EndMenu();
        }
    }

    void drawMainMenuToolsTab()
    {
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem(ICON_FA_PLAY " Simulate", "Ctrl+R"))
            {
                osc::ActionStartSimulatingModel(m_MainUIStateAPI, *m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_EDIT " Edit simulation settings"))
            {
                m_EditorAPI->pushPopup(std::make_unique<osc::ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI->updSimulationParams()));
            }

            if (ImGui::MenuItem("         Export Points"))
            {
                m_EditorAPI->pushPopup(std::make_unique<ExportPointsPopup>("Export Points", m_Model));
            }

            if (ImGui::BeginMenu("         Experimental Tools"))
            {
                if (ImGui::MenuItem("Simulate Against All Integrators (advanced)"))
                {
                    osc::ActionSimulateAgainstAllIntegrators(m_MainUIStateAPI, *m_Model);
                }

                osc::DrawTooltipIfItemHovered("Simulate Against All Integrators", "Simulate the given model against all available SimTK integrators. This takes the current simulation parameters and permutes the integrator, reporting the overall simulation wall-time to the user. It's an advanced feature that's handy for developers to figure out which integrator best-suits a particular model");
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }

    void drawMainMenuActionsTab()
    {
        if (ImGui::BeginMenu("Actions"))
        {
            if (ImGui::MenuItem("Disable all wrapping surfaces"))
            {
                osc::ActionDisableAllWrappingSurfaces(*m_Model);
            }

            if (ImGui::MenuItem("Enable all wrapping surfaces"))
            {
                osc::ActionEnableAllWrappingSurfaces(*m_Model);
            }

            ImGui::EndMenu();
        }
    }

    ParentPtr<MainUIStateAPI> m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<osc::UndoableModelStatePair> m_Model;
    MainMenuFileTab m_MainMenuFileTab;
    ModelActionsMenuItems m_MainMenuAddTabMenuItems{m_EditorAPI, m_Model};
    WindowMenu m_WindowMenu{m_EditorAPI->getPanelManager()};
    MainMenuAboutTab m_MainMenuAboutTab;
};


// public API (PIMPL)

osc::ModelEditorMainMenu::ModelEditorMainMenu(
    ParentPtr<MainUIStateAPI> const& mainStateAPI_,
    EditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(mainStateAPI_, editorAPI_, std::move(model_))}
{
}

osc::ModelEditorMainMenu::ModelEditorMainMenu(ModelEditorMainMenu&&) noexcept = default;
osc::ModelEditorMainMenu& osc::ModelEditorMainMenu::operator=(ModelEditorMainMenu&&) noexcept = default;
osc::ModelEditorMainMenu::~ModelEditorMainMenu() noexcept = default;

void osc::ModelEditorMainMenu::onDraw()
{
    m_Impl->onDraw();
}

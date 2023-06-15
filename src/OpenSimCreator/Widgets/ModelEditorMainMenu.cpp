#include "ModelEditorMainMenu.hpp"

#include "OpenSimCreator/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp"
#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Panels/ModelMusclePlotPanel.hpp"
#include "OpenSimCreator/Tabs/Experimental/ExcitationEditorTab.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Widgets/MainMenu.hpp"
#include "OpenSimCreator/Widgets/ModelActionsMenuItems.hpp"
#include "OpenSimCreator/Widgets/ParamBlockEditorPopup.hpp"
#include "OpenSimCreator/ActionFunctions.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Widgets/WindowMenu.hpp>
#include <oscar/Platform/Config.hpp>
#include <oscar/Platform/Styling.hpp>
#include <oscar/Utils/Algorithms.hpp>

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <memory>
#include <utility>

auto constexpr c_EditorScreenPanels = osc::MakeArray<char const* const>
(
    "Navigator",
    "Properties",
    "Log",
    "Coordinates",
    "Performance",
    "Output Watches"
);

class osc::ModelEditorMainMenu::Impl final {
public:
    Impl(
        std::weak_ptr<MainUIStateAPI> mainStateAPI_,
        EditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> model_) :

        m_MainUIStateAPI{std::move(mainStateAPI_)},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)}
    {
    }

    void draw()
    {
        m_MainMenuFileTab.draw(m_MainUIStateAPI, m_Model.get());
        drawMainMenuEditTab();
        drawMainMenuAddTab();
        drawMainMenuToolsTab();
        drawMainMenuActionsTab();
        m_WindowMenu.draw();
        m_MainMenuAboutTab.draw();
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

            if (ImGui::MenuItem("         Deselect", nullptr, false, m_Model->getSelected()))
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
            m_MainMenuAddTabMenuItems.draw();
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
                m_EditorAPI->pushPopup(std::make_unique<osc::ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI.lock()->updSimulationParams()));
            }

            if (ImGui::BeginMenu("         Experimental Tools"))
            {
                if (ImGui::MenuItem("Excitation Editor"))
                {
                    if (auto api = m_MainUIStateAPI.lock())
                    {
                        api->addAndSelectTab<ExcitationEditorTab>(
                            m_MainUIStateAPI,
                            m_Model
                        );
                    }
                }

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

    std::weak_ptr<MainUIStateAPI> m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<osc::UndoableModelStatePair> m_Model;
    MainMenuFileTab m_MainMenuFileTab;
    ModelActionsMenuItems m_MainMenuAddTabMenuItems{m_EditorAPI, m_Model};
    WindowMenu m_WindowMenu{m_EditorAPI->getPanelManager()};
    MainMenuAboutTab m_MainMenuAboutTab;
};


// public API (PIMPL)

osc::ModelEditorMainMenu::ModelEditorMainMenu(
    std::weak_ptr<MainUIStateAPI> mainStateAPI_,
    EditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(std::move(mainStateAPI_), editorAPI_, std::move(model_))}
{
}

osc::ModelEditorMainMenu::ModelEditorMainMenu(ModelEditorMainMenu&&) noexcept = default;
osc::ModelEditorMainMenu& osc::ModelEditorMainMenu::operator=(ModelEditorMainMenu&&) noexcept = default;
osc::ModelEditorMainMenu::~ModelEditorMainMenu() noexcept = default;

void osc::ModelEditorMainMenu::draw()
{
    m_Impl->draw();
}
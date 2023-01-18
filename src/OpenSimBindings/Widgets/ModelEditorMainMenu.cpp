#include "ModelEditorMainMenu.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/Panels/ModelMusclePlotPanel.hpp"
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/OpenSimBindings/Widgets/ModelActionsMenuItems.hpp"
#include "src/OpenSimBindings/Widgets/ParamBlockEditorPopup.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Widgets/WindowMenu.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/Algorithms.hpp"

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
        MainUIStateAPI* mainStateAPI,
        EditorAPI* editorAPI,
        std::shared_ptr<UndoableModelStatePair> model) :

        m_MainUIStateAPI{mainStateAPI},
        m_EditorAPI{editorAPI},
        m_Model{std::move(model)}
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
                osc::ActionStartSimulatingModel(*m_MainUIStateAPI, *m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_EDIT " Edit simulation settings"))
            {
                m_EditorAPI->pushPopup(std::make_unique<osc::ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI->updSimulationParams()));
            }

            if (ImGui::MenuItem("Simulate Against All Integrators (advanced)"))
            {
                osc::ActionSimulateAgainstAllIntegrators(*m_MainUIStateAPI, *m_Model);
            }
            osc::DrawTooltipIfItemHovered("Simulate Against All Integrators", "Simulate the given model against all available SimTK integrators. This takes the current simulation parameters and permutes the integrator, reporting the overall simulation wall-time to the user. It's an advanced feature that's handy for developers to figure out which integrator best-suits a particular model");

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

    MainUIStateAPI* m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<osc::UndoableModelStatePair> m_Model;
    MainMenuFileTab m_MainMenuFileTab;
    ModelActionsMenuItems m_MainMenuAddTabMenuItems{m_EditorAPI, m_Model};
    WindowMenu m_WindowMenu{m_EditorAPI->getPanelManager()};
    MainMenuAboutTab m_MainMenuAboutTab;
};


// public API (PIMPL)

osc::ModelEditorMainMenu::ModelEditorMainMenu(
    MainUIStateAPI* mainStateAPI,
    EditorAPI* editorAPI,
    std::shared_ptr<UndoableModelStatePair> model) :

    m_Impl{std::make_unique<Impl>(mainStateAPI, editorAPI, std::move(model))}
{
}

osc::ModelEditorMainMenu::ModelEditorMainMenu(ModelEditorMainMenu&&) noexcept = default;
osc::ModelEditorMainMenu& osc::ModelEditorMainMenu::operator=(ModelEditorMainMenu&&) noexcept = default;
osc::ModelEditorMainMenu::~ModelEditorMainMenu() noexcept = default;

void osc::ModelEditorMainMenu::draw()
{
    m_Impl->draw();
}
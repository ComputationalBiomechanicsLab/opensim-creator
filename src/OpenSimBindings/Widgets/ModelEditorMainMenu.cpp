#include "ModelEditorMainMenu.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/OpenSimBindings/Widgets/ModelActionsMenuItems.hpp"
#include "src/OpenSimBindings/Widgets/ModelMusclePlotPanel.hpp"
#include "src/OpenSimBindings/Widgets/ParamBlockEditorPopup.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
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
        drawMainMenuWindowTab();
        m_MainMenuAboutTab.draw();

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
        if (ImGui::Button(ICON_FA_PLAY " Simulate (Ctrl+R)"))
        {
            osc::ActionStartSimulatingModel(*m_MainUIStateAPI, *m_Model);
        }
        osc::App::upd().addFrameAnnotation("Simulate Button", osc::GetItemRect());
        ImGui::PopStyleColor();

        if (ImGui::Button(ICON_FA_EDIT " Edit simulation settings"))
        {
            m_EditorAPI->pushPopup(std::make_unique<osc::ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI->updSimulationParams()));
        }
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

            {
                float scaleFactor = m_Model->getFixupScaleFactor();
                if (ImGui::InputFloat("set scale factor", &scaleFactor))
                {
                    osc::ActionSetModelSceneScaleFactorTo(*m_Model, scaleFactor);
                }
            }

            if (ImGui::MenuItem(ICON_FA_EXPAND_ARROWS_ALT " autoscale scale factor"))
            {
                osc::ActionAutoscaleSceneScaleFactor(*m_Model);
            }
            osc::DrawTooltipIfItemHovered("Autoscale Scale Factor", "Try to autoscale the model's scale factor based on the current dimensions of the model");

            if (ImGui::MenuItem(ICON_FA_ARROWS_ALT " toggle frames"))
            {
                osc::ActionToggleFrames(*m_Model);
            }
            osc::DrawTooltipIfItemHovered("Toggle Frames", "Set the model's display properties to display physical frames");

            bool modelHasBackingFile = osc::HasInputFileName(m_Model->getModel());

            if (ImGui::MenuItem(ICON_FA_REDO " Reload osim", nullptr, false, modelHasBackingFile))
            {
                osc::ActionReloadOsimFromDisk(*m_Model);
            }
            osc::DrawTooltipIfItemHovered("Reload osim file", "Attempts to reload the osim file from scratch. This can be useful if (e.g.) editing third-party files that OpenSim Creator doesn't automatically track.");

            if (ImGui::MenuItem(ICON_FA_CLIPBOARD " Copy .osim path to clipboard", nullptr, false, modelHasBackingFile))
            {
                osc::ActionCopyModelPathToClipboard(*m_Model);
            }
            osc::DrawTooltipIfItemHovered("Copy .osim path to clipboard", "Copies the absolute path to the model's .osim file into your clipboard.\n\nThis is handy if you want to (e.g.) load the osim via a script, open it from the command line in an other app, etc.");

            if (ImGui::MenuItem(ICON_FA_FOLDER " Open .osim's parent directory", nullptr, false, modelHasBackingFile))
            {
                osc::ActionOpenOsimParentDirectory(*m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_LINK " Open .osim in external editor", nullptr, false, modelHasBackingFile))
            {
                osc::ActionOpenOsimInExternalEditor(*m_Model);
            }
            osc::DrawTooltipIfItemHovered("Open .osim in external editor", "Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");

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

            if (ImGui::MenuItem("Disable all wrapping surfaces"))
            {
                osc::ActionDisableAllWrappingSurfaces(*m_Model);
            }

            if (ImGui::MenuItem("Enable all wrapping surfaces"))
            {
                osc::ActionEnableAllWrappingSurfaces(*m_Model);
            }

            if (ImGui::MenuItem("Simulate Against All Integrators (advanced)"))
            {
                osc::ActionSimulateAgainstAllIntegrators(*m_MainUIStateAPI, *m_Model);
            }
            osc::DrawTooltipIfItemHovered("Simulate Against All Integrators", "Simulate the given model against all available SimTK integrators. This takes the current simulation parameters and permutes the integrator, reporting the overall simulation wall-time to the user. It's an advanced feature that's handy for developers to figure out which integrator best-suits a particular model");

            ImGui::EndMenu();
        }
    }

    void drawMainMenuWindowTab()
    {
        // draw "window" tab
        if (ImGui::BeginMenu("Window"))
        {
            osc::Config const& cfg = osc::App::get().getConfig();
            for (auto const& panel : c_EditorScreenPanels)
            {
                bool currentVal = cfg.getIsPanelEnabled(panel);
                if (ImGui::MenuItem(panel, nullptr, &currentVal))
                {
                    osc::App::upd().updConfig().setIsPanelEnabled(panel, currentVal);
                }
            }

            ImGui::Separator();

            // active 3D viewers (can be disabled)
            for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(m_EditorAPI->getNumModelVisualizers()); ++i)
            {
                std::string const name = m_EditorAPI->getModelVisualizerName(i);

                bool enabled = true;
                if (ImGui::MenuItem(name.c_str(), nullptr, &enabled))
                {
                    m_EditorAPI->deleteVisualizer(i);
                    --i;
                }
            }

            if (ImGui::MenuItem("add viewer"))
            {
                m_EditorAPI->addVisualizer();
            }

            ImGui::Separator();

            // active muscle plots
            for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(m_EditorAPI->getNumMusclePlots()); ++i)
            {
                osc::ModelMusclePlotPanel const& plot = m_EditorAPI->getMusclePlot(i);

                bool enabled = true;
                if (!plot.isOpen() || ImGui::MenuItem(plot.getName().c_str(), nullptr, &enabled))
                {
                    m_EditorAPI->deleteMusclePlot(i);
                    --i;
                }
            }

            if (ImGui::MenuItem("add muscle plot"))
            {
                m_EditorAPI->addEmptyMusclePlot();
            }

            ImGui::EndMenu();
        }
    }

    MainUIStateAPI* m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<osc::UndoableModelStatePair> m_Model;
    MainMenuFileTab m_MainMenuFileTab;
    ModelActionsMenuItems m_MainMenuAddTabMenuItems{m_EditorAPI, m_Model};
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
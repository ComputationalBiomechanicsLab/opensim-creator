#include "ModelEditorMainMenu.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/ModelEditor/ExportPointsPopup.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/UI/ModelEditor/ModelActionsMenuItems.h>
#include <OpenSimCreator/UI/ModelEditor/ModelMusclePlotPanel.h>
#include <OpenSimCreator/UI/Shared/ImportStationsFromCSVPopup.h>
#include <OpenSimCreator/UI/Shared/MainMenu.h>
#include <OpenSimCreator/UI/Shared/ParamBlockEditorPopup.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/Utils/ParentPtr.h>

#include <memory>
#include <utility>

class osc::ModelEditorMainMenu::Impl final {
public:
    Impl(
        ParentPtr<IMainUIStateAPI> const& mainStateAPI_,
        IEditorAPI* editorAPI_,
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
        if (ui::BeginMenu("Edit"))
        {
            if (ui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, m_Model->canUndo()))
            {
                ActionUndoCurrentlyEditedModel(*m_Model);
            }

            if (ui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, m_Model->canRedo()))
            {
                ActionRedoCurrentlyEditedModel(*m_Model);
            }

            ui::Separator();

            if (ui::MenuItem("         Deselect", {}, false, m_Model->getSelected() != nullptr))
            {
                m_Model->setSelected(nullptr);
            }

            ui::EndMenu();
        }
    }

    void drawMainMenuAddTab()
    {
        if (ui::BeginMenu("Add"))
        {
            m_MainMenuAddTabMenuItems.onDraw();
            ui::EndMenu();
        }
    }

    void drawMainMenuToolsTab()
    {
        if (ui::BeginMenu("Tools"))
        {
            if (ui::MenuItem(ICON_FA_PLAY " Simulate", "Ctrl+R"))
            {
                ActionStartSimulatingModel(m_MainUIStateAPI, *m_Model);
            }

            if (ui::MenuItem(ICON_FA_EDIT " Edit simulation settings"))
            {
                m_EditorAPI->pushPopup(std::make_unique<ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI->updSimulationParams()));
            }

            if (ui::MenuItem("         Import Points"))
            {
                m_EditorAPI->pushPopup(std::make_unique<ImportStationsFromCSVPopup>(
                    "Import Points",
                    [model = m_Model](auto lms)
                    {
                        ActionImportLandmarks(*model, lms.landmarks, lms.maybeLabel);
                    }
                ));
            }

            if (ui::MenuItem("         Export Points"))
            {
                m_EditorAPI->pushPopup(std::make_unique<ExportPointsPopup>("Export Points", m_Model));
            }

            if (ui::BeginMenu("         Experimental Tools"))
            {
                if (ui::MenuItem("Simulate Against All Integrators (advanced)"))
                {
                    ActionSimulateAgainstAllIntegrators(m_MainUIStateAPI, *m_Model);
                }
                ui::DrawTooltipIfItemHovered("Simulate Against All Integrators", "Simulate the given model against all available SimTK integrators. This takes the current simulation parameters and permutes the integrator, reporting the overall simulation wall-time to the user. It's an advanced feature that's handy for developers to figure out which integrator best-suits a particular model");

                if (ui::MenuItem("Export Model Graph as Dotviz")) {
                    ActionExportModelGraphToDotviz(*m_Model);
                }
                ui::DrawTooltipIfItemHovered("Writes the model's data topology graph in dotviz format, so that it can be visualized in external tooling such as Graphviz Online");

                if (ui::MenuItem("Export Model Graph as Dotviz (clipboard)")) {
                    ActionExportModelGraphToDotvizClipboard(*m_Model);
                }

                ui::EndMenu();
            }

            ui::EndMenu();
        }
    }

    void drawMainMenuActionsTab()
    {
        if (ui::BeginMenu("Actions"))
        {
            if (ui::MenuItem("Disable all wrapping surfaces"))
            {
                ActionDisableAllWrappingSurfaces(*m_Model);
            }

            if (ui::MenuItem("Enable all wrapping surfaces"))
            {
                ActionEnableAllWrappingSurfaces(*m_Model);
            }

            ui::EndMenu();
        }
    }

    ParentPtr<IMainUIStateAPI> m_MainUIStateAPI;
    IEditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
    MainMenuFileTab m_MainMenuFileTab;
    ModelActionsMenuItems m_MainMenuAddTabMenuItems{m_EditorAPI, m_Model};
    WindowMenu m_WindowMenu{m_EditorAPI->getPanelManager()};
    MainMenuAboutTab m_MainMenuAboutTab;
};


// public API (PIMPL)

osc::ModelEditorMainMenu::ModelEditorMainMenu(
    ParentPtr<IMainUIStateAPI> const& mainStateAPI_,
    IEditorAPI* editorAPI_,
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

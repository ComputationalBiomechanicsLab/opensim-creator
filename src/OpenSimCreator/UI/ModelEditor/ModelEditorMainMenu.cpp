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
#include <OpenSimCreator/UI/PerformanceAnalyzerTab.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/Utils/ParentPtr.h>

#include <memory>
#include <utility>

using namespace osc;

namespace
{
    bool ActionSimulateAgainstAllIntegrators(
        const ParentPtr<IMainUIStateAPI>& parent,
        const IModelStatePair& model)
    {
        parent->add_and_select_tab<PerformanceAnalyzerTab>(
            parent,
            BasicModelStatePair{model},
            parent->getSimulationParams()
        );
        return true;
    }
}

class osc::ModelEditorMainMenu::Impl final {
public:
    Impl(
        const ParentPtr<IMainUIStateAPI>& mainStateAPI_,
        IEditorAPI* editorAPI_,
        std::shared_ptr<IModelStatePair> model_) :

        m_MainUIStateAPI{mainStateAPI_},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)}
    {}

    void onDraw()
    {
        m_MainMenuFileTab.onDraw(m_MainUIStateAPI, m_Model.get());
        drawMainMenuEditTab();
        drawMainMenuAddTab();
        drawMainMenuToolsTab();
        drawMainMenuActionsTab();
        m_WindowMenu.on_draw();
        m_MainMenuAboutTab.onDraw();
    }

private:
    void drawMainMenuEditTab()
    {
        if (ui::begin_menu("Edit")) {
            auto* undoable = dynamic_cast<UndoableModelStatePair*>(m_Model.get());
            if (ui::draw_menu_item(OSC_ICON_UNDO " Undo", "Ctrl+Z", false, undoable and undoable->canUndo())) {
                if (undoable) {
                    undoable->doUndo();
                }
            }

            if (ui::draw_menu_item(OSC_ICON_REDO " Redo", "Ctrl+Shift+Z", false, undoable and undoable->canRedo())) {
                if (undoable) {
                    undoable->doRedo();
                }
            }

            ui::draw_separator();

            if (ui::draw_menu_item("         Deselect", {}, false, m_Model->getSelected() != nullptr)) {
                m_Model->setSelected(nullptr);
            }

            ui::end_menu();
        }
    }

    void drawMainMenuAddTab()
    {
        if (ui::begin_menu("Add")) {
            m_MainMenuAddTabMenuItems.onDraw();
            ui::end_menu();
        }
    }

    void drawMainMenuToolsTab()
    {
        if (ui::begin_menu("Tools")) {
            if (ui::draw_menu_item(OSC_ICON_PLAY " Simulate", "Ctrl+R")) {
                ActionStartSimulatingModel(m_MainUIStateAPI, *m_Model);
            }

            if (ui::draw_menu_item(OSC_ICON_EDIT " Edit simulation settings")) {
                m_EditorAPI->pushPopup(std::make_unique<ParamBlockEditorPopup>("simulation parameters", &m_MainUIStateAPI->updSimulationParams()));
            }

            if (ui::draw_menu_item("         Import Points")) {
                m_EditorAPI->pushPopup(std::make_unique<ImportStationsFromCSVPopup>(
                    "Import Points",
                    [model = m_Model](auto lms)
                    {
                        ActionImportLandmarks(*model, lms.landmarks, lms.maybeLabel);
                    }
                ));
            }

            if (ui::draw_menu_item("         Export Points")) {
                m_EditorAPI->pushPopup(std::make_unique<ExportPointsPopup>("Export Points", m_Model));
            }

            if (ui::begin_menu("         Experimental Tools")) {
                if (ui::draw_menu_item("Simulate Against All Integrators (advanced)")) {
                    ActionSimulateAgainstAllIntegrators(m_MainUIStateAPI, *m_Model);
                }
                ui::draw_tooltip_if_item_hovered("Simulate Against All Integrators", "Simulate the given model against all available SimTK integrators. This takes the current simulation parameters and permutes the integrator, reporting the overall simulation wall-time to the user. It's an advanced feature that's handy for developers to figure out which integrator best-suits a particular model");

                if (ui::draw_menu_item("Export Model Graph as Dotviz")) {
                    ActionExportModelGraphToDotviz(*m_Model);
                }
                ui::draw_tooltip_if_item_hovered("Writes the model's data topology graph in dotviz format, so that it can be visualized in external tooling such as Graphviz Online");

                if (ui::draw_menu_item("Export Model Graph as Dotviz (clipboard)")) {
                    ActionExportModelGraphToDotvizClipboard(*m_Model);
                }

                if (ui::draw_menu_item("Export Model Multibody System as Dotviz (clipboard)")) {
                    ActionExportModelMultibodySystemAsDotviz(*m_Model);
                }
                ui::draw_tooltip_if_item_hovered("Writes the model's multibody system (kinematic chain) in dotviz format, so that it can be visualized in external tooling such as Graphviz Online");

                ui::end_menu();
            }

            ui::end_menu();
        }
    }

    void drawMainMenuActionsTab()
    {
        if (ui::begin_menu("Actions")) {
            if (ui::draw_menu_item("Disable all wrapping surfaces")) {
                ActionDisableAllWrappingSurfaces(*m_Model);
            }

            if (ui::draw_menu_item("Enable all wrapping surfaces")) {
                ActionEnableAllWrappingSurfaces(*m_Model);
            }

            ui::end_menu();
        }
    }

    ParentPtr<IMainUIStateAPI> m_MainUIStateAPI;
    IEditorAPI* m_EditorAPI;
    std::shared_ptr<IModelStatePair> m_Model;
    MainMenuFileTab m_MainMenuFileTab;
    ModelActionsMenuItems m_MainMenuAddTabMenuItems{m_EditorAPI, m_Model};
    WindowMenu m_WindowMenu{m_EditorAPI->getPanelManager()};
    MainMenuAboutTab m_MainMenuAboutTab;
};


osc::ModelEditorMainMenu::ModelEditorMainMenu(
    const ParentPtr<IMainUIStateAPI>& mainStateAPI_,
    IEditorAPI* editorAPI_,
    std::shared_ptr<IModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(mainStateAPI_, editorAPI_, std::move(model_))}
{}
osc::ModelEditorMainMenu::ModelEditorMainMenu(ModelEditorMainMenu&&) noexcept = default;
osc::ModelEditorMainMenu& osc::ModelEditorMainMenu::operator=(ModelEditorMainMenu&&) noexcept = default;
osc::ModelEditorMainMenu::~ModelEditorMainMenu() noexcept = default;

void osc::ModelEditorMainMenu::onDraw()
{
    m_Impl->onDraw();
}

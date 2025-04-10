#include "ModelEditorMainMenu.h"

#include <libopensimcreator/Documents/Model/Environment.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/UI/ModelEditor/ExportPointsPopup.h>
#include <libopensimcreator/UI/ModelEditor/ModelActionsMenuItems.h>
#include <libopensimcreator/UI/ModelEditor/ModelMusclePlotPanel.h>
#include <libopensimcreator/UI/PerformanceAnalyzerTab.h>
#include <libopensimcreator/UI/Shared/ImportStationsFromCSVPopup.h>
#include <libopensimcreator/UI/Shared/MainMenu.h>
#include <libopensimcreator/UI/Shared/ParamBlockEditorPopup.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/UI/Events/OpenPopupEvent.h>
#include <liboscar/UI/Events/OpenTabEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Widgets/WindowMenu.h>
#include <liboscar/Utils/LifetimedPtr.h>

#include <memory>
#include <utility>

using namespace osc;

namespace
{
    bool ActionSimulateAgainstAllIntegrators(
        Widget& parent,
        const IModelStatePair& model)
    {
        auto tab = std::make_unique<PerformanceAnalyzerTab>(
            &parent,
            BasicModelStatePair{model},
            model.tryUpdEnvironment()->getSimulationParams()
        );
        App::post_event<OpenTabEvent>(parent, std::move(tab));
        return true;
    }
}

class osc::ModelEditorMainMenu::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner_,
        Widget* parent_,
        std::shared_ptr<PanelManager> panelManager_,
        std::shared_ptr<IModelStatePair> model_) :

        WidgetPrivate{owner_, parent_},
        m_Model{std::move(model_)},
        m_MainMenuFileTab{&owner_},
        m_WindowMenu{&owner_, std::move(panelManager_)}
    {}

    void onDraw()
    {
        m_MainMenuFileTab.onDraw(m_Model);
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
            if (ui::draw_menu_item(OSC_ICON_UNDO " Undo", KeyModifier::Ctrl | Key::Z, false, undoable != nullptr and undoable->canUndo())) {
                if (undoable) {
                    undoable->doUndo();
                }
            }

            if (ui::draw_menu_item(OSC_ICON_REDO " Redo", KeyModifier::Ctrl | KeyModifier::Shift | Key::Z, false, undoable != nullptr and undoable->canRedo())) {
                if (undoable) {
                    undoable->doRedo();
                }
            }

            ui::draw_separator();

            if (ui::draw_menu_item("         Deselect", Key::Escape, false, m_Model->getSelected() != nullptr)) {
                m_Model->clearSelected();
            }

            ui::end_menu();
        }
    }

    void drawMainMenuAddTab()
    {
        if (ui::begin_menu("Add")) {
            m_MainMenuAddTabMenuItems.on_draw();
            ui::end_menu();
        }
    }

    void drawMainMenuToolsTab()
    {
        if (ui::begin_menu("Tools")) {
            if (ui::draw_menu_item(OSC_ICON_PLAY " Simulate", KeyModifier::Ctrl | Key::R)) {
                ActionStartSimulatingModel(owner(), *m_Model);
            }

            if (ui::draw_menu_item(OSC_ICON_EDIT " Edit simulation settings")) {
                if (parent()) {
                    auto popup = std::make_unique<ParamBlockEditorPopup>(
                        &owner(),
                        "simulation parameters",
                        &m_Model->tryUpdEnvironment()->updSimulationParams()
                    );
                    App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
                }
            }

            if (ui::draw_menu_item("         Import Points", {}, nullptr, m_Model->canUpdModel())) {
                if (parent()) {
                    auto popup = std::make_unique<ImportStationsFromCSVPopup>(
                        &owner(),
                        "Import Points",
                        [model = m_Model](auto lms)
                        {
                            ActionImportLandmarks(*model, lms.landmarks, lms.maybeLabel);
                        }
                    );
                    App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
                }
            }

            if (ui::draw_menu_item("         Export Points")) {
                auto popup = std::make_unique<ExportPointsPopup>(&owner(), "Export Points", m_Model);
                if (parent()) {
                    App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
                }
            }

            if (ui::begin_menu("         Experimental Tools")) {
                if (ui::draw_menu_item("Simulate Against All Integrators (advanced)")) {
                    if (parent()) {
                        ActionSimulateAgainstAllIntegrators(*parent(), *m_Model);
                    }
                }
                ui::draw_tooltip_if_item_hovered("Simulate Against All Integrators", "Simulate the given model against all available SimTK integrators. This takes the current simulation parameters and permutes the integrator, reporting the overall simulation wall-time to the user. It's an advanced feature that's handy for developers to figure out which integrator best-suits a particular model");

                if (ui::draw_menu_item("Export Model Graph as Dotviz")) {
                    ActionExportModelGraphToDotviz(m_Model);
                }
                ui::draw_tooltip_if_item_hovered("Writes the model's data topology graph in dotviz format, so that it can be visualized in external tooling such as Graphviz Online");

                if (ui::draw_menu_item("Export Model Graph as Dotviz (clipboard)")) {
                    ActionExportModelGraphToDotvizClipboard(*m_Model);
                }

                if (ui::draw_menu_item("Export Model Multibody System as Dotviz (clipboard)")) {
                    ActionExportModelMultibodySystemAsDotviz(*m_Model);
                }
                ui::draw_tooltip_if_item_hovered("Writes the model's multibody system (kinematic chain) in dotviz format, so that it can be visualized in external tooling such as Graphviz Online");

                if (ui::draw_menu_item("WIP: Bake Station Defined Frames")) {
                    ActionBakeStationDefinedFrames(*m_Model);
                }
                ui::draw_tooltip_if_item_hovered("WORK IN PROGRESS (WIP): Converts any `StationDefinedFrame`s in the model into `PhysicalOffsetFrame`s. Effectively, \"baking\" the current (station-defined) frame transform.\n\nThe main reason to do this is backwards compatibility, OpenSim <= v4.5 doesn't have native support for `StationDefinedFrame`s (later versions should: see opensim-core/#3694)");

                ui::end_menu();
            }

            ui::end_menu();
        }
    }

    void drawMainMenuActionsTab()
    {
        if (ui::begin_menu("Actions")) {
            if (ui::draw_menu_item("Disable all wrapping surfaces", {}, nullptr, m_Model->canUpdModel())) {
                ActionDisableAllWrappingSurfaces(*m_Model);
            }

            if (ui::draw_menu_item("Enable all wrapping surfaces", {}, nullptr, m_Model->canUpdModel())) {
                ActionEnableAllWrappingSurfaces(*m_Model);
            }

            ui::end_menu();
        }
    }

    std::shared_ptr<IModelStatePair> m_Model;
    MainMenuFileTab m_MainMenuFileTab;
    ModelActionsMenuItems m_MainMenuAddTabMenuItems{&owner(), m_Model};
    WindowMenu m_WindowMenu;
    MainMenuAboutTab m_MainMenuAboutTab;
};


osc::ModelEditorMainMenu::ModelEditorMainMenu(
    Widget* parent_,
    std::shared_ptr<PanelManager> panelManager_,
    std::shared_ptr<IModelStatePair> model_) :

    Widget{std::make_unique<Impl>(*this, parent_, std::move(panelManager_), std::move(model_))}
{}

void osc::ModelEditorMainMenu::impl_on_draw()
{
    private_data().onDraw();
}

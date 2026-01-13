#include "ModelEditorToolbar.h"

#include <libopensimcreator/Documents/Model/Environment.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Shared/ParamBlockEditorPopup.h>

#include <liboscar/graphics/Color.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/platform/App.h>
#include <liboscar/platform/Widget.h>
#include <liboscar/platform/WidgetPrivate.h>
#include <liboscar/ui/events/OpenPopupEvent.h>
#include <liboscar/ui/IconCache.h>
#include <liboscar/ui/oscimgui.h>

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>

class osc::ModelEditorToolbar::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner_,
        Widget* parent_,
        std::string_view label_,
        std::shared_ptr<UndoableModelStatePair> model_) :

        WidgetPrivate{owner_, parent_},
        m_Model{std::move(model_)}
    {
        set_name(label_);
    }

    void onDraw()
    {
        if (BeginToolbar(name(), Vector2{5.0f, 5.0f})) {
            drawContent();
        }
        ui::end_panel();
    }

    OSC_OWNER_GETTERS(ModelEditorToolbar);
private:
    void drawModelFileRelatedButtons()
    {
        DrawNewModelButton(owner());
        ui::same_line();
        DrawOpenModelButtonWithRecentFilesDropdown(owner());
        ui::same_line();
        DrawSaveModelButton(m_Model);
        ui::same_line();
        DrawReloadModelButton(*m_Model);
    }

    void drawForwardDynamicSimulationControls()
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 0.0f});

        ui::push_style_color(ui::ColorVar::Text, Color::dark_green());
        if (ui::draw_button(MSMICONS_PLAY)) {
            if (parent()) {
                ActionStartSimulatingModel(*parent(), *m_Model);
            }
        }
        ui::pop_style_color();
        ui::add_screenshot_annotation_to_last_drawn_item("Simulate Button");
        ui::draw_tooltip_if_item_hovered("Simulate Model", "Run a forward-dynamic simulation of the model");

        ui::same_line();

        if (ui::draw_button(MSMICONS_EDIT))
        {
            auto popup = std::make_unique<ParamBlockEditorPopup>(
                &owner(),
                "simulation parameters",
                &m_Model->tryUpdEnvironment()->updSimulationParams()
            );
            App::post_event<OpenPopupEvent>(owner(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Edit Simulation Settings", "Change the parameters used when simulating the model");

        ui::pop_style_var();
    }

    void drawContent()
    {
        drawModelFileRelatedButtons();
        ui::draw_same_line_with_vertical_separator();

        DrawUndoAndRedoButtons(*m_Model);
        ui::draw_same_line_with_vertical_separator();

        DrawSceneScaleFactorEditorControls(*m_Model);
        ui::draw_same_line_with_vertical_separator();

        drawForwardDynamicSimulationControls();
        ui::draw_same_line_with_vertical_separator();

        DrawAllDecorationToggleButtons(*m_Model, *m_IconCache);
    }

    std::shared_ptr<UndoableModelStatePair> m_Model;

    std::shared_ptr<IconCache> m_IconCache = App::singleton<IconCache>(
        App::resource_loader().with_prefix("OpenSimCreator/icons/"),
        ui::get_font_base_size()/128.0f,
        App::get().highest_device_pixel_ratio()
    );
};

osc::ModelEditorToolbar::ModelEditorToolbar(
    Widget* parent_,
    std::string_view label_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    Widget{std::make_unique<Impl>(*this, parent_, label_, std::move(model_))}
{}

void osc::ModelEditorToolbar::impl_on_draw()
{
    private_data().onDraw();
}

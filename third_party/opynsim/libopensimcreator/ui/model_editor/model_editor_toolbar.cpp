#include "model_editor_toolbar.h"

#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/param_block_editor_popup.h>

#include <liboscar/graphics/color.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/icon_cache.h>
#include <liboscar/ui/oscimgui.h>

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>

class osc::ModelEditorToolbar::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner,
        Widget* parent,
        std::string_view label,
        std::shared_ptr<UndoableModelStatePair> model) :

        WidgetPrivate{owner, parent},
        model_{std::move(model)}
    {
        set_name(label);
    }

    void on_draw()
    {
        if (begin_toolbar(name(), Vector2{5.0f, 5.0f})) {
            draw_content();
        }
        ui::end_panel();
    }

    OSC_OWNER_GETTERS(ModelEditorToolbar);

private:
    void draw_model_file_related_buttons()
    {
        draw_new_model_button(owner());
        ui::same_line();
        draw_open_model_button_with_recent_files_dropdown(owner());
        ui::same_line();
        draw_save_model_button(model_);
        ui::same_line();
        draw_reload_model_button(*model_);
    }

    void draw_forward_dynamic_simulation_controls()
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 0.0f});

        ui::push_style_color(ui::ColorVar::Text, Color::dark_green());
        if (ui::draw_button(MSMICONS_PLAY)) {
            if (parent()) {
                action_start_simulating_model(*parent(), *model_);
            }
        }
        ui::pop_style_color();
        ui::add_screenshot_annotation_to_last_drawn_item("Simulate Button");
        ui::draw_tooltip_if_item_hovered(
            "Simulate Model", "Run a forward-dynamic simulation of the model");

        ui::same_line();

        if (ui::draw_button(MSMICONS_EDIT)) {
            auto popup = std::make_unique<ParamBlockEditorPopup>(
                &owner(),
                "simulation parameters",
                &model_->try_upd_environment()->upd_simulation_params());
            App::post_event<OpenPopupEvent>(owner(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered(
            "Edit Simulation Settings", "Change the parameters used when simulating the model");

        ui::pop_style_var();
    }

    void draw_content()
    {
        draw_model_file_related_buttons();
        ui::draw_same_line_with_vertical_separator();

        draw_undo_and_redo_buttons(*model_);
        ui::draw_same_line_with_vertical_separator();

        draw_scene_scale_factor_editor_controls(*model_);
        ui::draw_same_line_with_vertical_separator();

        draw_forward_dynamic_simulation_controls();
        ui::draw_same_line_with_vertical_separator();

        draw_all_decoration_toggle_buttons(*model_, *icon_cache_);
    }

    std::shared_ptr<UndoableModelStatePair> model_;

    std::shared_ptr<IconCache> icon_cache_ = App::singleton<IconCache>(
        App::resource_loader().with_prefix("OpenSimCreator/icons/"),
        ui::get_font_base_size() / 128.0f,
        App::get().highest_device_pixel_ratio());
};

osc::ModelEditorToolbar::ModelEditorToolbar(
    Widget* parent, std::string_view label, std::shared_ptr<UndoableModelStatePair> model) :

    Widget{std::make_unique<Impl>(*this, parent, label, std::move(model))}
{}

void osc::ModelEditorToolbar::impl_on_draw()
{
    private_data().on_draw();
}

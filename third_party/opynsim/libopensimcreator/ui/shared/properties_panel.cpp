#include "properties_panel.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/open_component_context_menu_event.h>
#include <libopensimcreator/ui/shared/object_properties_editor.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/color.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utilities/scope_exit.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    void draw_actions_menu(Widget& parent, const std::shared_ptr<opyn::ModelStatePair>& model)
    {
        const OpenSim::Component* const selection = model->get_selected();
        if (not selection) {
            return;
        }

        ui::set_num_columns(2);
        ui::align_text_to_frame_padding();  // ensure it aligns with the button in the next column
        ui::draw_text("actions");
        ui::same_line();
        ui::draw_help_marker(
            "Shows a menu containing extra actions that can be performed on this component.\n\nYou can also access the same menu by right-clicking the component in the 3D viewer, bottom status bar, or navigator panel.");
        ui::next_column();
        ui::push_style_color(ui::ColorVar::Text, Color::yellow());
        if (ui::draw_button(MSMICONS_BOLT) or ui::is_item_clicked(MouseButton::Right)) {
            App::post_event<OpenComponentContextMenuEvent>(
                parent, opyn::get_absolute_path(*selection));
        }
        ui::pop_style_color();
        ui::next_column();
        ui::set_num_columns();
    }

    class ObjectNameEditor final {
    public:
        explicit ObjectNameEditor(std::shared_ptr<opyn::ModelStatePair> model) :
            model_{std::move(model)}
        {}

        void on_draw()
        {
            const OpenSim::Component* const selected = model_->get_selected();
            if (not selected) {
                return;  // don't do anything if nothing is selected
            }

            // update cached edits if model/selection changes
            if (model_->get_model_version() != last_model_version_ or selected != last_selected_) {
                edited_name_ = selected->getName();
                last_model_version_ = model_->get_model_version();
                last_selected_ = selected;
            }

            const bool disabled = model_->is_readonly();
            if (disabled) {
                ui::begin_disabled();
            }

            ui::set_num_columns(2);

            ui::draw_separator();
            ui::align_text_to_frame_padding();  // ensure it aligns with the next column
            ui::draw_text("name");
            ui::same_line();
            ui::draw_help_marker(
                "The name of the component",
                "The component's name can be important. It can be used when components want to refer to eachover. E.g. a joint will name the two frames it attaches to.");

            ui::next_column();

            ui::set_next_item_width(ui::get_content_region_available().x());
            ui::draw_string_input("##nameeditor", edited_name_);
            if (ui::should_save_last_drawn_item_value()) {
                action_set_component_name(
                    *model_, opyn::get_absolute_path(*selected), edited_name_);
            }
            ui::add_screenshot_annotation_to_last_drawn_item("PropertiesPanel/name");

            ui::next_column();

            ui::set_num_columns();

            if (disabled) {
                ui::end_disabled();
            }
        }

    private:
        std::shared_ptr<opyn::ModelStatePair> model_;
        UID last_model_version_;
        const OpenSim::Component* last_selected_ = nullptr;
        std::string edited_name_;
    };
}

class osc::PropertiesPanel::Impl final : public PanelPrivate {
public:
    explicit Impl(
        PropertiesPanel& owner,
        Widget* parent,
        std::string_view panel_name,
        std::shared_ptr<opyn::ModelStatePair> model) :

        PanelPrivate{owner, parent, panel_name},
        model_{std::move(model)},
        selection_properties_editor_{&owner, model_, [model = model_] {
                                         return model->get_selected();
                                     }}
    {}

    void draw_content()
    {
        if (not model_->get_selected()) {
            ui::draw_text_disabled_and_panel_centered("(nothing selected)");
            return;
        }

        ui::push_id(model_->get_selected());
        const ScopeExit g{[] {
            ui::pop_id();
        }};

        // draw an actions row with a button that opens the context menu
        //
        // it's helpful to reveal to users that actions are available (#426)
        draw_actions_menu(*parent(), model_);

        name_editor_.on_draw();

        if (not model_->get_selected()) {
            return;
        }

        // property editors
        {
            auto maybe_updater = selection_properties_editor_.on_draw();
            if (maybe_updater) {
                action_apply_property_edit(*model_, *maybe_updater);
            }
        }
    }

private:
    std::shared_ptr<opyn::ModelStatePair> model_;
    ObjectNameEditor name_editor_{model_};
    ObjectPropertiesEditor selection_properties_editor_;
};

osc::PropertiesPanel::PropertiesPanel(
    Widget* parent, std::string_view panel_name, std::shared_ptr<opyn::ModelStatePair> model) :
    Panel{std::make_unique<Impl>(*this, parent, panel_name, std::move(model))}
{}
void osc::PropertiesPanel::impl_draw_content()
{
    private_data().draw_content();
}

#include "model_status_bar.h"

#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/component_context_menu.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/string_helpers.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <utility>

class osc::ModelStatusBar::Impl final : public WidgetPrivate {
public:
    explicit Impl(Widget& owner, Widget* parent, std::shared_ptr<opyn::ModelStatePair> model) :

        WidgetPrivate{owner, parent},
        model_{std::move(model)}
    {}

    void on_draw()
    {
        ui::begin_main_window_bottom_bar("bottom");
        draw_selection_breadcrumbs();
        ui::end_panel();
    }

private:
    void draw_selection_breadcrumbs()
    {
        const OpenSim::Component* const c = model_->get_selected();

        if (c) {
            const std::vector<const OpenSim::Component*> els = opyn::get_path_elements(*c);
            for (ptrdiff_t i = 0; i < std::ssize(els) - 1; ++i) {
                const OpenSim::Component& el = *els[i];

                ui::push_id(i);
                ui::draw_text(icon_for(el));
                ui::same_line();
                const std::string label = truncate_with_ellipsis(el.getName(), 15);
                if (ui::draw_small_button(label)) {
                    model_->set_selected(&el);
                }
                draw_mouse_interaction_stuff(el);
                ui::same_line();
                ui::draw_text_disabled("/");
                ui::same_line();
                ui::pop_id();
            }
            if (not els.empty()) {
                ui::draw_text(icon_for(*els.back()));
                ui::same_line();
                const std::string label = truncate_with_ellipsis(els.back()->getName(), 15);
                ui::draw_text(label);
                draw_mouse_interaction_stuff(*els.back());
            }
        } else {
            ui::draw_text_disabled("(nothing selected)");
        }
    }

    void draw_mouse_interaction_stuff(const OpenSim::Component& c)
    {
        if (ui::is_item_hovered()) {
            model_->set_hovered(&c);

            ui::begin_tooltip();
            ui::draw_text_disabled(c.getConcreteClassName());
            ui::end_tooltip();
        }
        if (ui::is_item_clicked(MouseButton::Right)) {
            if (parent()) {
                auto menu = std::make_unique<ComponentContextMenu>(
                    parent(), "##hovermenu", model_, opyn::get_absolute_path(c));
                menu->open();
                App::post_event<OpenPopupEvent>(*parent(), std::move(menu));
            }
        }
    }

    std::shared_ptr<opyn::ModelStatePair> model_;
};

osc::ModelStatusBar::ModelStatusBar(Widget* parent, std::shared_ptr<opyn::ModelStatePair> model) :

    Widget{std::make_unique<Impl>(*this, parent, std::move(model))}
{}
void osc::ModelStatusBar::impl_on_draw()
{
    private_data().on_draw();
}

#include "ModelStatusBar.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Shared/ComponentContextMenu.h>

#include <libopynsim/Utils/OpenSimHelpers.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utils/string_helpers.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <utility>


class osc::ModelStatusBar::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner_,
        Widget* parent_,
        std::shared_ptr<IModelStatePair> model_) :

        WidgetPrivate{owner_, parent_},
        m_Model{std::move(model_)}
    {}

    void onDraw()
    {
        ui::begin_main_window_bottom_bar("bottom");
        drawSelectionBreadcrumbs();
        ui::end_panel();
    }

private:
    void drawSelectionBreadcrumbs()
    {
        const OpenSim::Component* const c = m_Model->getSelected();

        if (c) {
            const std::vector<const OpenSim::Component*> els = GetPathElements(*c);
            for (ptrdiff_t i = 0; i < std::ssize(els)-1; ++i) {
                const OpenSim::Component& el = *els[i];

                ui::push_id(i);
                ui::draw_text(IconFor(el));
                ui::same_line();
                const std::string label = truncate_with_ellipsis(el.getName(), 15);
                if (ui::draw_small_button(label)) {
                    m_Model->setSelected(&el);
                }
                drawMouseInteractionStuff(el);
                ui::same_line();
                ui::draw_text_disabled("/");
                ui::same_line();
                ui::pop_id();
            }
            if (not els.empty()) {
                ui::draw_text(IconFor(*els.back()));
                ui::same_line();
                const std::string label = truncate_with_ellipsis(els.back()->getName(), 15);
                ui::draw_text(label);
                drawMouseInteractionStuff(*els.back());
            }
        }
        else {
            ui::draw_text_disabled("(nothing selected)");
        }
    }

    void drawMouseInteractionStuff(const OpenSim::Component& c)
    {
        if (ui::is_item_hovered()) {
            m_Model->setHovered(&c);

            ui::begin_tooltip();
            ui::draw_text_disabled(c.getConcreteClassName());
            ui::end_tooltip();
        }
        if (ui::is_item_clicked(ui::MouseButton::Right)) {
            if (parent()) {
                auto menu = std::make_unique<ComponentContextMenu>(
                    parent(),
                    "##hovermenu",
                    m_Model,
                    GetAbsolutePath(c)
                );
                menu->open();
                App::post_event<OpenPopupEvent>(*parent(), std::move(menu));
            }
        }
    }

    std::shared_ptr<IModelStatePair> m_Model;
};


osc::ModelStatusBar::ModelStatusBar(
    Widget* parent_,
    std::shared_ptr<IModelStatePair> model_) :

    Widget{std::make_unique<Impl>(*this, parent_, std::move(model_))}
{}
void osc::ModelStatusBar::impl_on_draw() { private_data().onDraw(); }

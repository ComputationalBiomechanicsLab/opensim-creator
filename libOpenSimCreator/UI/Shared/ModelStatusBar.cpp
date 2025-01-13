#include "ModelStatusBar.h"

#include <libOpenSimCreator/Documents/Model/IModelStatePair.h>
#include <libOpenSimCreator/UI/Shared/BasicWidgets.h>
#include <libOpenSimCreator/UI/Shared/ComponentContextMenu.h>
#include <libOpenSimCreator/Utils/OpenSimHelpers.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/Events/OpenPopupEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/LifetimedPtr.h>
#include <liboscar/Utils/StringHelpers.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <utility>


class osc::ModelStatusBar::Impl final {
public:
    Impl(Widget& parent_, std::shared_ptr<IModelStatePair> model_) :

        m_Parent{parent_.weak_ref()},
        m_Model{std::move(model_)}
    {}

    void onDraw()
    {
        ui::begin_main_viewport_bottom_bar("bottom");
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
                ui::draw_text_unformatted(label);
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
            auto menu = std::make_unique<ComponentContextMenu>(
                "##hovermenu",
                *m_Parent,
                m_Model,
                GetAbsolutePath(c)
            );
            menu->open();
            App::post_event<OpenPopupEvent>(*m_Parent, std::move(menu));
        }
    }

    LifetimedPtr<Widget> m_Parent;
    std::shared_ptr<IModelStatePair> m_Model;
};


osc::ModelStatusBar::ModelStatusBar(
    Widget& parent_,
    std::shared_ptr<IModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(parent_, std::move(model_))}
{}
osc::ModelStatusBar::ModelStatusBar(ModelStatusBar&&) noexcept = default;
osc::ModelStatusBar& osc::ModelStatusBar::operator=(ModelStatusBar&&) noexcept = default;
osc::ModelStatusBar::~ModelStatusBar() noexcept = default;

void osc::ModelStatusBar::onDraw()
{
    m_Impl->onDraw();
}

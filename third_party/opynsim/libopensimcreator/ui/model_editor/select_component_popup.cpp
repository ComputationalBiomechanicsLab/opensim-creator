#include "select_component_popup.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <functional>
#include <memory>
#include <string_view>

class osc::SelectComponentPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        SelectComponentPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        std::shared_ptr<const opyn::ModelStatePair> model,
        std::function<void(const OpenSim::ComponentPath&)> on_selection,
        std::function<bool(const OpenSim::Component&)> filter) :

        PopupPrivate{owner, parent, popup_name},
        model_{std::move(model)},
        on_selection_{std::move(on_selection)},
        filter_{std::move(filter)}
    {}

    void draw_content()
    {
        const OpenSim::Component* selected = nullptr;

        // iterate through each T in `root` and give the user the option to click it
        {
            ui::begin_child_panel(
                "first",
                Vector2{256.0f, 256.0f},
                ui::ChildPanelFlag::Border,
                ui::PanelFlag::HorizontalScrollbar);
            for (const OpenSim::Component& c : model_->get_model().getComponentList()) {
                if (not filter_(c)) {
                    continue;  // filtered out
                }
                if (ui::draw_button(c.getName())) {
                    selected = &c;
                }
            }
            ui::end_child_panel();
        }

        if (selected) {
            on_selection_(opyn::get_absolute_path(*selected));
            request_close();
        }
    }

private:
    std::shared_ptr<const opyn::ModelStatePair> model_;
    std::function<void(const OpenSim::ComponentPath&)> on_selection_;
    std::function<bool(const OpenSim::Component&)> filter_;
};

osc::SelectComponentPopup::SelectComponentPopup(
    Widget* parent,
    std::string_view popup_name,
    std::shared_ptr<const opyn::ModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> on_selection,
    std::function<bool(const OpenSim::Component&)> filter) :

    Popup{std::make_unique<Impl>(
        *this, parent, popup_name, std::move(model), std::move(on_selection), std::move(filter))}
{}
void osc::SelectComponentPopup::impl_draw_content()
{
    private_data().draw_content();
}

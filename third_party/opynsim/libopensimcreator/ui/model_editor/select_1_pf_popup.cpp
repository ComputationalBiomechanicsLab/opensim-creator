#include "select_1_pf_popup.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>

#include <algorithm>
#include <string_view>

class osc::Select1PFPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        Select1PFPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        std::shared_ptr<const opyn::ModelStatePair> model,
        std::function<void(const OpenSim::ComponentPath&)> on_selection) :

        PopupPrivate{owner, parent, popup_name},
        model_{std::move(model)},
        on_selection_{std::move(on_selection)}
    {}

    void draw_content()
    {
        const OpenSim::PhysicalFrame* selected = nullptr;

        ui::begin_child_panel(
            "pflist",
            Vector2{256.0f, 256.0f},
            ui::ChildPanelFlag::Border,
            ui::PanelFlag::HorizontalScrollbar);
        for (const auto& pf : model_->get_model().getComponentList<OpenSim::PhysicalFrame>()) {
            if (ui::draw_selectable(pf.getName())) {
                selected = &pf;
            }
        }
        ui::end_child_panel();

        if (selected) {
            on_selection_(opyn::get_absolute_path(*selected));
            request_close();
        }
    }

private:
    std::shared_ptr<const opyn::ModelStatePair> model_;
    std::function<void(const OpenSim::ComponentPath&)> on_selection_;
};

osc::Select1PFPopup::Select1PFPopup(
    Widget* parent,
    std::string_view popup_name,
    std::shared_ptr<const opyn::ModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> on_selection) :

    Popup{std::make_unique<Impl>(
        *this, parent, popup_name, std::move(model), std::move(on_selection))}
{}
void osc::Select1PFPopup::impl_draw_content()
{
    private_data().draw_content();
}

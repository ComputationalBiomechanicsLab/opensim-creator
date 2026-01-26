#include "Select1PFPopup.h"

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
        std::string_view popupName,
        std::shared_ptr<const ModelStatePair> model,
        std::function<void(const OpenSim::ComponentPath&)> onSelection) :

        PopupPrivate{owner, parent, popupName},
        m_Model{std::move(model)},
        m_OnSelection{std::move(onSelection)}
    {}

    void draw_content()
    {
        const OpenSim::PhysicalFrame* selected = nullptr;

        ui::begin_child_panel("pflist", Vector2{256.0f, 256.0f}, ui::ChildPanelFlag::Border, ui::PanelFlag::HorizontalScrollbar);
        for (const auto& pf : m_Model->getModel().getComponentList<OpenSim::PhysicalFrame>()) {
            if (ui::draw_selectable(pf.getName())) {
                selected = &pf;
            }
        }
        ui::end_child_panel();

        if (selected) {
            m_OnSelection(opyn::GetAbsolutePath(*selected));
            request_close();
        }
    }

private:
    std::shared_ptr<const ModelStatePair> m_Model;
    std::function<void(const OpenSim::ComponentPath&)> m_OnSelection;
};


osc::Select1PFPopup::Select1PFPopup(
    Widget* parent,
    std::string_view popupName,
    std::shared_ptr<const ModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> onSelection) :

    Popup{std::make_unique<Impl>(*this, parent, popupName, std::move(model), std::move(onSelection))}
{}
void osc::Select1PFPopup::impl_draw_content() { private_data().draw_content(); }

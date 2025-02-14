#include "SelectComponentPopup.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Popups/Popup.h>
#include <liboscar/UI/Popups/PopupPrivate.h>
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
        std::string_view popupName,
        std::shared_ptr<const IModelStatePair> model,
        std::function<void(const OpenSim::ComponentPath&)> onSelection,
        std::function<bool(const OpenSim::Component&)> filter) :

        PopupPrivate{owner, parent, popupName},
        m_Model{std::move(model)},
        m_OnSelection{std::move(onSelection)},
        m_Filter{std::move(filter)}
    {}

    void draw_content()
    {
        const OpenSim::Component* selected = nullptr;

        // iterate through each T in `root` and give the user the option to click it
        {
            ui::begin_child_panel("first", Vec2{256.0f, 256.0f}, ui::ChildPanelFlag::Border, ui::PanelFlag::HorizontalScrollbar);
            for (const OpenSim::Component& c : m_Model->getModel().getComponentList())
            {
                if (!m_Filter(c))
                {
                    continue;  // filtered out
                }
                if (ui::draw_button(c.getName()))
                {
                    selected = &c;
                }
            }
            ui::end_child_panel();
        }

        if (selected)
        {
            m_OnSelection(GetAbsolutePath(*selected));
            request_close();
        }
    }

private:
    std::shared_ptr<const IModelStatePair> m_Model;
    std::function<void(const OpenSim::ComponentPath&)> m_OnSelection;
    std::function<bool(const OpenSim::Component&)> m_Filter;
};

osc::SelectComponentPopup::SelectComponentPopup(
    Widget* parent,
    std::string_view popupName,
    std::shared_ptr<const IModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> onSelection,
    std::function<bool(const OpenSim::Component&)> filter) :

    Popup{std::make_unique<Impl>(*this, parent, popupName, std::move(model), std::move(onSelection), std::move(filter))}
{}
void osc::SelectComponentPopup::impl_draw_content() { private_data().draw_content(); }

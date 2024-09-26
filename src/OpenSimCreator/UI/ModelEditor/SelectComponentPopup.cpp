#include "SelectComponentPopup.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>

#include <functional>
#include <memory>
#include <string_view>

class osc::SelectComponentPopup::Impl final : public StandardPopup {
public:
    Impl(std::string_view popupName,
         std::shared_ptr<const IModelStatePair> model,
         std::function<void(const OpenSim::ComponentPath&)> onSelection,
         std::function<bool(const OpenSim::Component&)> filter) :

        StandardPopup{popupName},
        m_Model{std::move(model)},
        m_OnSelection{std::move(onSelection)},
        m_Filter{std::move(filter)}
    {}

private:
    void impl_draw_content() final
    {
        const OpenSim::Component* selected = nullptr;

        // iterate through each T in `root` and give the user the option to click it
        {
            ui::begin_child_panel("first", Vec2{256.0f, 256.0f}, ui::ChildPanelFlag::Border, ui::WindowFlag::HorizontalScrollbar);
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

    std::shared_ptr<const IModelStatePair> m_Model;
    std::function<void(const OpenSim::ComponentPath&)> m_OnSelection;
    std::function<bool(const OpenSim::Component&)> m_Filter;
};

osc::SelectComponentPopup::SelectComponentPopup(
    std::string_view popupName,
    std::shared_ptr<const IModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> onSelection,
    std::function<bool(const OpenSim::Component&)> filter) :

    m_Impl{std::make_unique<Impl>(popupName, std::move(model), std::move(onSelection), std::move(filter))}
{}
osc::SelectComponentPopup::SelectComponentPopup(SelectComponentPopup&&) noexcept = default;
osc::SelectComponentPopup& osc::SelectComponentPopup::operator=(SelectComponentPopup&&) noexcept = default;
osc::SelectComponentPopup::~SelectComponentPopup() noexcept = default;

bool osc::SelectComponentPopup::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::SelectComponentPopup::impl_open()
{
    m_Impl->open();
}

void osc::SelectComponentPopup::impl_close()
{
    m_Impl->close();
}

bool osc::SelectComponentPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::SelectComponentPopup::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::SelectComponentPopup::impl_end_popup()
{
    m_Impl->end_popup();
}

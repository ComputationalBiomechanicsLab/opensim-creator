#include "Select1PFPopup.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>

#include <algorithm>
#include <string>

class osc::Select1PFPopup::Impl final : public StandardPopup {
public:

    Impl(std::string_view popupName,
         std::shared_ptr<const UndoableModelStatePair> model,
         std::function<void(const OpenSim::ComponentPath&)> onSelection) :

        StandardPopup{popupName},
        m_Model{std::move(model)},
        m_OnSelection{std::move(onSelection)}
    {
    }

private:
    void impl_draw_content() final
    {
        const OpenSim::PhysicalFrame* selected = nullptr;

        ui::begin_child_panel("pflist", Vec2{256.0f, 256.0f}, ImGuiChildFlags_Border, ui::WindowFlag::HorizontalScrollbar);
        for (const auto& pf : m_Model->getModel().getComponentList<OpenSim::PhysicalFrame>())
        {
            if (ui::draw_selectable(pf.getName()))
            {
                selected = &pf;
            }
        }
        ui::end_child_panel();

        if (selected)
        {
            m_OnSelection(GetAbsolutePath(*selected));
            request_close();
        }
    }

    std::shared_ptr<const UndoableModelStatePair> m_Model;
    std::function<void(const OpenSim::ComponentPath&)> m_OnSelection;
};


osc::Select1PFPopup::Select1PFPopup(
    std::string_view popupName,
    std::shared_ptr<const UndoableModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> onSelection) :

    m_Impl{std::make_unique<Impl>(popupName, std::move(model), std::move(onSelection))}
{}
osc::Select1PFPopup::Select1PFPopup(Select1PFPopup&&) noexcept = default;
osc::Select1PFPopup& osc::Select1PFPopup::operator=(Select1PFPopup&&) noexcept = default;
osc::Select1PFPopup::~Select1PFPopup() noexcept = default;

bool osc::Select1PFPopup::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::Select1PFPopup::impl_open()
{
    m_Impl->open();
}

void osc::Select1PFPopup::impl_close()
{
    m_Impl->close();
}

bool osc::Select1PFPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::Select1PFPopup::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::Select1PFPopup::impl_end_popup()
{
    m_Impl->end_popup();
}

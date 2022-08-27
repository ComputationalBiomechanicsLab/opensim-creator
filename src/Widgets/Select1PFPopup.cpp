#include "Select1PFPopup.hpp"

#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Widgets/StandardPopup.hpp"

#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>

#include <algorithm>
#include <string>

class osc::Select1PFPopup::Impl final : public StandardPopup {
public:

    Impl(std::string_view popupName,
         std::shared_ptr<UndoableModelStatePair> model,
         std::function<void(OpenSim::ComponentPath const&)> onSelection) :

        StandardPopup{std::move(popupName)},
        m_Model{std::move(model)},
        m_OnSelection{std::move(onSelection)}
    {
    }

private:
    void implDraw() override
    {
        OpenSim::PhysicalFrame const* selected = nullptr;

        ImGui::BeginChild("pflist", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (auto const& pf : m_Model->getModel().getComponentList<OpenSim::PhysicalFrame>())
        {
            if (ImGui::Selectable(pf.getName().c_str()))
            {
                selected = &pf;
            }
        }
        ImGui::EndChild();

        if (selected)
        {
            m_OnSelection(selected->getAbsolutePath());
            requestClose();
        }
    }

    std::shared_ptr<UndoableModelStatePair> m_Model;
    std::function<void(OpenSim::ComponentPath const&)> m_OnSelection;
};


// public API (PIMPL)

osc::Select1PFPopup::Select1PFPopup(
    std::string_view popupName,
    std::shared_ptr<UndoableModelStatePair> model,
    std::function<void(OpenSim::ComponentPath const&)> onSelection) :

    m_Impl{new Impl{std::move(popupName), std::move(model), std::move(onSelection)}}
{
}

osc::Select1PFPopup::Select1PFPopup(Select1PFPopup&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::Select1PFPopup& osc::Select1PFPopup::operator=(Select1PFPopup&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::Select1PFPopup::~Select1PFPopup() noexcept
{
    delete m_Impl;
}

bool osc::Select1PFPopup::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::Select1PFPopup::open()
{
    m_Impl->open();
}

void osc::Select1PFPopup::close()
{
    m_Impl->close();
}

void osc::Select1PFPopup::draw()
{
    m_Impl->draw();
}

#include "Select1PFPopup.hpp"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <oscar/UI/Widgets/StandardPopup.hpp>

#include <algorithm>
#include <string>

class osc::Select1PFPopup::Impl final : public StandardPopup {
public:

    Impl(std::string_view popupName,
         std::shared_ptr<UndoableModelStatePair const> model,
         std::function<void(OpenSim::ComponentPath const&)> onSelection) :

        StandardPopup{popupName},
        m_Model{std::move(model)},
        m_OnSelection{std::move(onSelection)}
    {
    }

private:
    void implDrawContent() final
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
            m_OnSelection(GetAbsolutePath(*selected));
            requestClose();
        }
    }

    std::shared_ptr<UndoableModelStatePair const> m_Model;
    std::function<void(OpenSim::ComponentPath const&)> m_OnSelection;
};


// public API (PIMPL)

osc::Select1PFPopup::Select1PFPopup(
    std::string_view popupName,
    std::shared_ptr<UndoableModelStatePair const> model,
    std::function<void(OpenSim::ComponentPath const&)> onSelection) :

    m_Impl{std::make_unique<Impl>(popupName, std::move(model), std::move(onSelection))}
{
}

osc::Select1PFPopup::Select1PFPopup(Select1PFPopup&&) noexcept = default;
osc::Select1PFPopup& osc::Select1PFPopup::operator=(Select1PFPopup&&) noexcept = default;
osc::Select1PFPopup::~Select1PFPopup() noexcept = default;

bool osc::Select1PFPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::Select1PFPopup::implOpen()
{
    m_Impl->open();
}

void osc::Select1PFPopup::implClose()
{
    m_Impl->close();
}

bool osc::Select1PFPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::Select1PFPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::Select1PFPopup::implEndPopup()
{
    m_Impl->endPopup();
}

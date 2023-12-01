#include "SelectComponentPopup.hpp"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/Widgets/StandardPopup.hpp>

#include <functional>
#include <memory>
#include <string_view>

class osc::SelectComponentPopup::Impl final : public StandardPopup {
public:
    Impl(std::string_view popupName,
         std::shared_ptr<UndoableModelStatePair const> model,
         std::function<void(OpenSim::ComponentPath const&)> onSelection,
         std::function<bool(OpenSim::Component const&)> filter) :

        StandardPopup{popupName},
        m_Model{std::move(model)},
        m_OnSelection{std::move(onSelection)},
        m_Filter{std::move(filter)}
    {
    }

private:
    void implDrawContent() final
    {
        OpenSim::Component const* selected = nullptr;

        // iterate through each T in `root` and give the user the option to click it
        {
            ImGui::BeginChild("first", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (OpenSim::Component const& c : m_Model->getModel().getComponentList())
            {
                if (!m_Filter(c))
                {
                    continue;  // filtered out
                }
                if (ImGui::Button(c.getName().c_str()))
                {
                    selected = &c;
                }
            }
            ImGui::EndChild();
        }

        if (selected)
        {
            m_OnSelection(osc::GetAbsolutePath(*selected));
            requestClose();
        }
    }

    std::shared_ptr<UndoableModelStatePair const> m_Model;
    std::function<void(OpenSim::ComponentPath const&)> m_OnSelection;
    std::function<bool(OpenSim::Component const&)> m_Filter;
};

osc::SelectComponentPopup::SelectComponentPopup(
    std::string_view popupName,
    std::shared_ptr<UndoableModelStatePair const> model,
    std::function<void(OpenSim::ComponentPath const&)> onSelection,
    std::function<bool(OpenSim::Component const&)> filter) :

    m_Impl{std::make_unique<Impl>(popupName, std::move(model), std::move(onSelection), std::move(filter))}
{
}

osc::SelectComponentPopup::SelectComponentPopup(SelectComponentPopup&&) noexcept = default;
osc::SelectComponentPopup& osc::SelectComponentPopup::operator=(SelectComponentPopup&&) noexcept = default;
osc::SelectComponentPopup::~SelectComponentPopup() noexcept = default;

bool osc::SelectComponentPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::SelectComponentPopup::implOpen()
{
    m_Impl->open();
}

void osc::SelectComponentPopup::implClose()
{
    m_Impl->close();
}

bool osc::SelectComponentPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::SelectComponentPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::SelectComponentPopup::implEndPopup()
{
    m_Impl->endPopup();
}

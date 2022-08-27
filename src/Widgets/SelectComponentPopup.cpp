#include "SelectComponentPopup.hpp"

#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Widgets/StandardPopup.hpp"

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <functional>
#include <memory>
#include <string_view>

class osc::SelectComponentPopup::Impl final : public StandardPopup {
public:
    Impl(std::string_view popupName,
         std::shared_ptr<UndoableModelStatePair> model,
         std::function<void(OpenSim::ComponentPath const&)> onSelection,
         std::function<bool(OpenSim::Component const&)> filter) :

        StandardPopup{std::move(popupName)},
        m_Model{std::move(model)},
        m_OnSelection{std::move(onSelection)},
        m_Filter{std::move(filter)}
    {
    }

private:
    void implDraw() override
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
            m_OnSelection(selected->getAbsolutePath());
            requestClose();
        }
    }

    std::shared_ptr<UndoableModelStatePair> m_Model;
    std::function<void(OpenSim::ComponentPath const&)> m_OnSelection;
    std::function<bool(OpenSim::Component const&)> m_Filter;
};

osc::SelectComponentPopup::SelectComponentPopup(
    std::string_view popupName,
    std::shared_ptr<UndoableModelStatePair> model,
    std::function<void(OpenSim::ComponentPath const&)> onSelection,
    std::function<bool(OpenSim::Component const&)> filter) :

    m_Impl{new Impl{std::move(popupName), std::move(model), std::move(onSelection), std::move(filter)}}
{
}

osc::SelectComponentPopup::SelectComponentPopup(SelectComponentPopup&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SelectComponentPopup& osc::SelectComponentPopup::operator=(SelectComponentPopup&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::SelectComponentPopup::~SelectComponentPopup() noexcept
{
    delete m_Impl;
}

bool osc::SelectComponentPopup::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::SelectComponentPopup::open()
{
    m_Impl->open();
}

void osc::SelectComponentPopup::close()
{
    m_Impl->close();
}

bool osc::SelectComponentPopup::beginPopup()
{
    return m_Impl->beginPopup();
}

void osc::SelectComponentPopup::drawPopupContent()
{
    m_Impl->drawPopupContent();
}

void osc::SelectComponentPopup::endPopup()
{
    m_Impl->endPopup();
}

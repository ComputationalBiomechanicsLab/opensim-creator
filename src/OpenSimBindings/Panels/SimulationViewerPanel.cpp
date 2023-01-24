#include "SimulationViewerPanel.hpp"

#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/UiModelViewer.hpp"
#include "src/OpenSimBindings/VirtualModelStatePair.hpp"
#include "src/Panels/StandardPanel.hpp"

#include <OpenSim/Common/Component.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::SimulationViewerPanel::Impl final : public StandardPanel {
public:
    Impl(
        std::string_view panelName,
        std::shared_ptr<VirtualModelStatePair> modelState,
        MainUIStateAPI* mainUIStateAPI) :

        StandardPanel{std::move(panelName)},
        m_Model{std::move(modelState)},
        m_API{mainUIStateAPI}
    {
    }

private:
    void implBeforeImGuiBegin() final
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    }

    void implAfterImGuiBegin() final
    {
        ImGui::PopStyleVar();
    }

    void implDrawContent() final
    {
        auto resp = m_Viewer.draw(*m_Model);

        // upate hover
        if (resp.isMousedOver && resp.hovertestResult != m_Model->getHovered())
        {
            m_Model->setHovered(resp.hovertestResult);
        }

        // if left-clicked, update selection (can be empty)
        if (m_Viewer.isLeftClicked() && resp.isMousedOver)
        {
            m_Model->setSelected(resp.hovertestResult);
        }

        // if hovered, draw hover tooltip
        if (resp.isMousedOver && resp.hovertestResult)
        {
            osc::DrawComponentHoverTooltip(*resp.hovertestResult);
        }

        // if right-clicked, draw a context menu
        {
            std::string menuName = std::string{getName()} + "_contextmenu";

            if (m_Viewer.isRightClicked() && resp.isMousedOver)
            {
                m_Model->setSelected(resp.hovertestResult);  // can be empty
                ImGui::OpenPopup(menuName.c_str());
            }

            OpenSim::Component const* selected = m_Model->getSelected();

            if (selected && ImGui::BeginPopup(menuName.c_str()))
            {
                // draw context menu for whatever's selected
                ImGui::TextUnformatted(selected->getName().c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("%s", selected->getConcreteClassName().c_str());
                ImGui::Separator();
                ImGui::Dummy({0.0f, 3.0f});

                DrawSelectOwnerMenu(*m_Model, *selected);
                DrawWatchOutputMenu(*m_API, *selected);
                ImGui::EndPopup();
            }
        }
    }

    std::shared_ptr<VirtualModelStatePair> m_Model;
    MainUIStateAPI* m_API;
    UiModelViewer m_Viewer;
};

osc::SimulationViewerPanel::SimulationViewerPanel(
    std::string_view panelName,
    std::shared_ptr<VirtualModelStatePair> modelState,
    MainUIStateAPI* mainUIStateAPI) :

    m_Impl{std::make_unique<Impl>(std::move(panelName), std::move(modelState), std::move(mainUIStateAPI))}
{
}
osc::SimulationViewerPanel::SimulationViewerPanel(SimulationViewerPanel&&) noexcept = default;
osc::SimulationViewerPanel& osc::SimulationViewerPanel::operator=(SimulationViewerPanel&&) noexcept = default;
osc::SimulationViewerPanel::~SimulationViewerPanel() noexcept = default;

osc::CStringView osc::SimulationViewerPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::SimulationViewerPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::SimulationViewerPanel::implOpen()
{
    m_Impl->open();
}

void osc::SimulationViewerPanel::implClose()
{
    m_Impl->close();
}

void osc::SimulationViewerPanel::implDraw()
{
    m_Impl->draw();
}
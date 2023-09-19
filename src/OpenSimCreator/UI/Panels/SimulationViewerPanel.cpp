#include "SimulationViewerPanel.hpp"

#include <OpenSimCreator/Model/VirtualModelStatePair.hpp>
#include <OpenSimCreator/UI/Panels/SimulationViewerPanelParameters.hpp>
#include <OpenSimCreator/UI/Panels/SimulationViewerRightClickEvent.hpp>
#include <OpenSimCreator/UI/Widgets/BasicWidgets.hpp>
#include <OpenSimCreator/UI/Widgets/UiModelViewer.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/Panels/StandardPanel.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    osc::SimulationViewerRightClickEvent MakeRightClickEvent(
        OpenSim::Component const* maybeHover)
    {
        std::optional<std::string> maybeAbsPath = maybeHover ?
            std::optional<std::string>{osc::GetAbsolutePathString(*maybeHover)} :
            std::optional<std::string>{};

        return osc::SimulationViewerRightClickEvent{std::move(maybeAbsPath)};
    }
}

class osc::SimulationViewerPanel::Impl final : public StandardPanel {
public:
    Impl(
        std::string_view panelName_,
        SimulationViewerPanelParameters params_) :

        StandardPanel{panelName_},
        m_Params{std::move(params_)}
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
        VirtualModelStatePair& msp = m_Params.updModelState();

        std::optional<SceneCollision> const maybeCollision = m_Viewer.onDraw(msp);

        OpenSim::Component const* maybeHover = maybeCollision ?
            osc::FindComponent(msp.getModel(), maybeCollision->decorationID) :
            nullptr;

        // care: this code must check whether the hover != current hover (even if
        // null), because there might be multiple viewports open (#582)
        if (m_Viewer.isMousedOver() && maybeHover != msp.getHovered())
        {
            // hovering: update hover and show tooltip
            msp.setHovered(maybeHover);
        }

        if (m_Viewer.isMousedOver() && m_Viewer.isLeftClicked())
        {
            msp.setSelected(maybeHover);
        }

        if (maybeHover)
        {
            DrawComponentHoverTooltip(*maybeHover);
        }

        if (m_Viewer.isMousedOver() && m_Viewer.isRightClicked())
        {
            m_Params.callOnRightClickHandler(MakeRightClickEvent(maybeHover));
        }
    }

    SimulationViewerPanelParameters m_Params;
    UiModelViewer m_Viewer;
};


// public API

osc::SimulationViewerPanel::SimulationViewerPanel(
    std::string_view panelName_,
    SimulationViewerPanelParameters params_) :

    m_Impl{std::make_unique<Impl>(panelName_, std::move(params_))}
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

void osc::SimulationViewerPanel::implOnDraw()
{
    m_Impl->onDraw();
}

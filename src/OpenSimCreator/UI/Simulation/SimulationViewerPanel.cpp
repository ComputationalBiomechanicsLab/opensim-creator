#include "SimulationViewerPanel.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/Readonly3DModelViewer.h>
#include <OpenSimCreator/UI/Simulation/SimulationViewerPanelParameters.h>
#include <OpenSimCreator/UI/Simulation/SimulationViewerRightClickEvent.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    SimulationViewerRightClickEvent MakeRightClickEvent(
        OpenSim::Component const* maybeHover)
    {
        std::optional<std::string> maybeAbsPath = maybeHover ?
            std::optional<std::string>{GetAbsolutePathString(*maybeHover)} :
            std::optional<std::string>{};

        return SimulationViewerRightClickEvent{std::move(maybeAbsPath)};
    }
}

class osc::SimulationViewerPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panelName_,
        SimulationViewerPanelParameters params_) :

        StandardPanelImpl{panelName_},
        m_Params{std::move(params_)},
        m_Viewer{panelName_}
    {
    }

private:
    void implBeforeImGuiBegin() final
    {
        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    }

    void implAfterImGuiBegin() final
    {
        ui::PopStyleVar();
    }

    void implDrawContent() final
    {
        IModelStatePair& msp = m_Params.updModelState();

        std::optional<SceneCollision> const maybeCollision = m_Viewer.onDraw(msp);

        OpenSim::Component const* maybeHover = maybeCollision ?
            FindComponent(msp.getModel(), maybeCollision->decorationID) :
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
    Readonly3DModelViewer m_Viewer;
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

CStringView osc::SimulationViewerPanel::implGetName() const
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

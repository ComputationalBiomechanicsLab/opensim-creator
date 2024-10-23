#include "SourceModelViewerPanel.h"

#include <OpenSimCreator/UI/ModelWarper/UIState.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanel.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanelParameters.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

osc::mow::SourceModelViewerPanel::SourceModelViewerPanel(
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :

    ModelViewerPanel{panelName_, ModelViewerPanelParameters{state_->modelstatePtr()}, ModelViewerPanelFlag::NoHittest},
    m_State{std::move(state_)}
{}

void osc::mow::SourceModelViewerPanel::impl_on_draw()
{
    if (m_State->isCameraLinked()) {
        if (m_State->isOnlyCameraRotationLinked()) {
            auto camera = getCamera();
            camera.phi = m_State->getLinkedCamera().phi;
            camera.theta = m_State->getLinkedCamera().theta;
            setCamera(camera);
        }
        else {
            setCamera(m_State->getLinkedCamera());
        }
    }

    setModelState(m_State->modelstatePtr());
    ModelViewerPanel::impl_on_draw();

    // draw may have updated the camera, so flash is back
    if (m_State->isCameraLinked()) {
        if (m_State->isOnlyCameraRotationLinked()) {
            auto camera = m_State->getLinkedCamera();
            camera.phi = getCamera().phi;
            camera.theta = getCamera().theta;
            m_State->setLinkedCamera(camera);
        }
        else {
            m_State->setLinkedCamera(getCamera());
        }
    }
}

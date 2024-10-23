#include "ResultModelViewerPanel.h"

#include <OpenSimCreator/UI/ModelWarper/UIState.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanel.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanelParameters.h>

#include <oscar/UI/oscimgui.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace osc;
using namespace osc::mow;

osc::mow::ResultModelViewerPanel::ResultModelViewerPanel(
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :

    ModelViewerPanel{panelName_, ModelViewerPanelParameters{state_->modelstatePtr()}, ModelViewerPanelFlag::NoHittest},
    m_State{std::move(state_)}
{}

void osc::mow::ResultModelViewerPanel::impl_on_draw()
{
    if (auto warped = m_State->tryGetWarpedModel()) {
        // handle camera linking
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

        setModelState(warped);
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
    else {
        ui::draw_text("cannot show result: model is not warpable");
    }
}

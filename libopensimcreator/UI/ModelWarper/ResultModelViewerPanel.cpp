#include "ResultModelViewerPanel.h"

#include <libopensimcreator/UI/ModelWarper/UIState.h>
#include <libopensimcreator/UI/Shared/ModelViewerPanel.h>
#include <libopensimcreator/UI/Shared/ModelViewerPanelParameters.h>

#include <liboscar/UI/oscimgui.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace osc;
using namespace osc::mow;

osc::mow::ResultModelViewerPanel::ResultModelViewerPanel(
    Widget* parent_,
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :

    ModelViewerPanel{parent_, panelName_, ModelViewerPanelParameters{state_->modelstatePtr()}, ModelViewerPanelFlag::NoHittest},
    m_State{std::move(state_)}
{}

void osc::mow::ResultModelViewerPanel::impl_draw_content()
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
        ModelViewerPanel::impl_draw_content();

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

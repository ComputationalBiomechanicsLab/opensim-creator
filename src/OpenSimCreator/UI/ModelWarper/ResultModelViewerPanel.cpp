#include "ResultModelViewerPanel.h"

#include <OpenSimCreator/UI/ModelWarper/UIState.h>
#include <OpenSimCreator/UI/Shared/Readonly3DModelViewer.h>

#include <oscar/UI.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

class osc::mow::ResultModelViewerPanel::Impl final : public StandardPanelImpl {
public:
    Impl(std::string_view panelName_,
        std::shared_ptr<UIState> state_) :

        StandardPanelImpl{panelName_},
        m_State{std::move(state_)}
    {}

private:
    void impl_before_imgui_begin() final
    {
        ui::push_style_var(ui::StyleVar::WindowPadding, {0.0f, 0.0f});
    }

    void impl_after_imgui_begin() final
    {
        ui::pop_style_var();
    }

    void impl_draw_content() final
    {
        if (auto warped = m_State->tryGetWarpedModel()) {
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = m_ModelViewer.getCamera();
                    camera.phi = m_State->getLinkedCamera().phi;
                    camera.theta = m_State->getLinkedCamera().theta;
                    m_ModelViewer.setCamera(camera);
                }
                else {
                    m_ModelViewer.setCamera(m_State->getLinkedCamera());
                }
            }

            m_ModelViewer.onDraw(*warped);

            // draw may have updated the camera, so flash is back
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = m_State->getLinkedCamera();
                    camera.phi = m_ModelViewer.getCamera().phi;
                    camera.theta = m_ModelViewer.getCamera().theta;
                    m_State->setLinkedCamera(camera);
                }
                else {
                    m_State->setLinkedCamera(m_ModelViewer.getCamera());
                }
            }
        }
        else {
            ui::draw_text("cannot show result: model is not warpable");
        }
    }

    std::shared_ptr<UIState> m_State;
    Readonly3DModelViewer m_ModelViewer{this->name(), Readonly3DModelViewerFlags::NoSceneHittest};
};

osc::mow::ResultModelViewerPanel::ResultModelViewerPanel(
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :
    m_Impl{std::make_unique<Impl>(panelName_, std::move(state_))}
{}

osc::mow::ResultModelViewerPanel::ResultModelViewerPanel(ResultModelViewerPanel&&) noexcept = default;
osc::mow::ResultModelViewerPanel& osc::mow::ResultModelViewerPanel::operator=(ResultModelViewerPanel&&) noexcept = default;
osc::mow::ResultModelViewerPanel::~ResultModelViewerPanel() noexcept = default;

CStringView osc::mow::ResultModelViewerPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::mow::ResultModelViewerPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::mow::ResultModelViewerPanel::impl_open()
{
    m_Impl->open();
}

void osc::mow::ResultModelViewerPanel::impl_close()
{
    m_Impl->close();
}

void osc::mow::ResultModelViewerPanel::impl_on_draw()
{
    m_Impl->on_draw();
}

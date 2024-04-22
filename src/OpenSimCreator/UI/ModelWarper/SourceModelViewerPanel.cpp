#include "SourceModelViewerPanel.h"

#include <OpenSimCreator/UI/ModelWarper/UIState.h>
#include <OpenSimCreator/UI/Shared/Readonly3DModelViewer.h>

#include <oscar/UI.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

class osc::mow::SourceModelViewerPanel::Impl final : public StandardPanelImpl {
public:
    Impl(std::string_view panelName_,
        std::shared_ptr<UIState> state_) :

        StandardPanelImpl{panelName_},
        m_State{std::move(state_)}
    {}

private:
    void impl_before_imgui_begin() final
    {
        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    }

    void impl_after_imgui_begin() final
    {
        ui::PopStyleVar();
    }

    void impl_draw_content() final
    {
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

        m_ModelViewer.onDraw(m_State->modelstate());

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

    std::shared_ptr<UIState> m_State;
    Readonly3DModelViewer m_ModelViewer{this->name(), Readonly3DModelViewerFlags::NoSceneHittest};
};

osc::mow::SourceModelViewerPanel::SourceModelViewerPanel(
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :
    m_Impl{std::make_unique<Impl>(panelName_, std::move(state_))}
{
}

osc::mow::SourceModelViewerPanel::SourceModelViewerPanel(SourceModelViewerPanel&&) noexcept = default;
osc::mow::SourceModelViewerPanel& osc::mow::SourceModelViewerPanel::operator=(SourceModelViewerPanel&&) noexcept = default;
osc::mow::SourceModelViewerPanel::~SourceModelViewerPanel() noexcept = default;

CStringView osc::mow::SourceModelViewerPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::mow::SourceModelViewerPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::mow::SourceModelViewerPanel::impl_open()
{
    m_Impl->open();
}

void osc::mow::SourceModelViewerPanel::impl_close()
{
    m_Impl->close();
}

void osc::mow::SourceModelViewerPanel::impl_on_draw()
{
    m_Impl->on_draw();
}

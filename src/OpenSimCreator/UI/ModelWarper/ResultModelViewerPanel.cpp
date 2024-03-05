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
        if (auto warped = m_State->tryGetWarpedModel()) {
            m_ModelViewer.onDraw(*warped);
        }
        else {
            ui::Text("cannot show result: model is not warpable");
        }
    }

    std::shared_ptr<UIState> m_State;
    Readonly3DModelViewer m_ModelViewer{this->getName(), Readonly3DModelViewerFlags::NoSceneHittest};
};

osc::mow::ResultModelViewerPanel::ResultModelViewerPanel(
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :
    m_Impl{std::make_unique<Impl>(panelName_, std::move(state_))}
{}

osc::mow::ResultModelViewerPanel::ResultModelViewerPanel(ResultModelViewerPanel&&) noexcept = default;
osc::mow::ResultModelViewerPanel& osc::mow::ResultModelViewerPanel::operator=(ResultModelViewerPanel&&) noexcept = default;
osc::mow::ResultModelViewerPanel::~ResultModelViewerPanel() noexcept = default;

CStringView osc::mow::ResultModelViewerPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::mow::ResultModelViewerPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::mow::ResultModelViewerPanel::implOpen()
{
    m_Impl->open();
}

void osc::mow::ResultModelViewerPanel::implClose()
{
    m_Impl->close();
}

void osc::mow::ResultModelViewerPanel::implOnDraw()
{
    m_Impl->onDraw();
}

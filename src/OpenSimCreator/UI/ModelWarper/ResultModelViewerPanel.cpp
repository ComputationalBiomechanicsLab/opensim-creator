#include "ResultModelViewerPanel.h"

#include <OpenSimCreator/UI/ModelWarper/UIState.h>

#include <imgui.h>
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
    void implDrawContent() final
    {
        if (m_State->state() == ValidationState::Ok) {
            ImGui::Text("ok!");
        }
        else {
            TextCentered("cannot show - model is not warpable");
        }
    }

    std::shared_ptr<UIState> m_State;
};

osc::mow::ResultModelViewerPanel::ResultModelViewerPanel(
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :
    m_Impl{std::make_unique<Impl>(panelName_, std::move(state_))}
{
}

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

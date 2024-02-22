#include "SourceModelViewerPanel.h"

#include <OpenSimCreator/UI/ModelWarper/UIState.h>

#include <imgui.h>
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

osc::mow::SourceModelViewerPanel::SourceModelViewerPanel(
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :
    m_Impl{std::make_unique<Impl>(panelName_, std::move(state_))}
{
}

osc::mow::SourceModelViewerPanel::SourceModelViewerPanel(SourceModelViewerPanel&&) noexcept = default;
osc::mow::SourceModelViewerPanel& osc::mow::SourceModelViewerPanel::operator=(SourceModelViewerPanel&&) noexcept = default;
osc::mow::SourceModelViewerPanel::~SourceModelViewerPanel() noexcept = default;

CStringView osc::mow::SourceModelViewerPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::mow::SourceModelViewerPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::mow::SourceModelViewerPanel::implOpen()
{
    m_Impl->open();
}

void osc::mow::SourceModelViewerPanel::implClose()
{
    m_Impl->close();
}

void osc::mow::SourceModelViewerPanel::implOnDraw()
{
    m_Impl->onDraw();
}

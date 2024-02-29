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
        m_ModelViewer.onDraw(m_State->modelstate());
    }

    std::shared_ptr<UIState> m_State;
    Readonly3DModelViewer m_ModelViewer{this->getName()};
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

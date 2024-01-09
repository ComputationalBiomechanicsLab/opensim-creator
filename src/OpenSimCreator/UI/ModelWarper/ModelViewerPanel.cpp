#include "ModelViewerPanel.hpp"

#include <OpenSimCreator/UI/ModelWarper/UIState.hpp>

#include <imgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.hpp>

#include <memory>
#include <string_view>
#include <utility>

using osc::CStringView;

class osc::mow::ModelViewerPanel::Impl final : public StandardPanelImpl {
public:
    Impl(std::string_view panelName_,
        std::shared_ptr<UIState> state_) :

        StandardPanelImpl{panelName_},
        m_State{std::move(state_)}
    {
    }
private:
    void implDrawContent() final
    {
        ImGui::Text("todo");
    }

    std::shared_ptr<UIState> m_State;
};

osc::mow::ModelViewerPanel::ModelViewerPanel(
    std::string_view panelName_,
    std::shared_ptr<UIState> state_) :
    m_Impl{std::make_unique<Impl>(panelName_, std::move(state_))}
{
}

osc::mow::ModelViewerPanel::ModelViewerPanel(ModelViewerPanel&&) noexcept = default;
osc::mow::ModelViewerPanel& osc::mow::ModelViewerPanel::operator=(ModelViewerPanel&&) noexcept = default;
osc::mow::ModelViewerPanel::~ModelViewerPanel() noexcept = default;

CStringView osc::mow::ModelViewerPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::mow::ModelViewerPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::mow::ModelViewerPanel::implOpen()
{
    m_Impl->open();
}

void osc::mow::ModelViewerPanel::implClose()
{
    m_Impl->close();
}

void osc::mow::ModelViewerPanel::implOnDraw()
{
    m_Impl->onDraw();
}

#include "LogViewerPanel.hpp"

#include "src/Widgets/LogViewer.hpp"
#include "src/Widgets/NamedPanel.hpp"

#include <imgui.h>

#include <string_view>
#include <utility>

class osc::LogViewerPanel::Impl final : public NamedPanel {
public:

    Impl(std::string_view panelName) : NamedPanel{std::move(panelName), ImGuiWindowFlags_MenuBar}
    {
    }

    bool isOpen() const
    {
        return static_cast<NamedPanel const&>(*this).isOpen();
    }

    void open()
    {
        return static_cast<NamedPanel&>(*this).open();
    }

    void close()
    {
        return static_cast<NamedPanel&>(*this).close();
    }

    void draw()
    {
        static_cast<NamedPanel&>(*this).draw();
    }

private:
    void implDrawContent() override
    {
        m_Viewer.draw();
    }

    LogViewer m_Viewer;
};

osc::LogViewerPanel::LogViewerPanel(std::string_view panelName) :
    m_Impl{std::make_unique<Impl>(std::move(panelName))}
{
}

osc::LogViewerPanel::LogViewerPanel(LogViewerPanel&&) noexcept = default;
osc::LogViewerPanel& osc::LogViewerPanel::operator=(LogViewerPanel&&) noexcept = default;
osc::LogViewerPanel::~LogViewerPanel() noexcept = default;

bool osc::LogViewerPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::LogViewerPanel::implOpen()
{
    m_Impl->open();
}

void osc::LogViewerPanel::implClose()
{
    m_Impl->close();
}

void osc::LogViewerPanel::implDraw()
{
    m_Impl->draw();
}
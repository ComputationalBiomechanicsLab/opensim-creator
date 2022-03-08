#include "PerfPanel.hpp"

#include <string>
#include <memory>
#include <utility>

class osc::PerfPanel::Impl final {
public:
    Impl(std::string_view panelName) :
        m_PanelName{std::move(panelName)}
    {
    }

    void open()
    {
    }

    void close()
    {
    }

    bool draw()
    {
        return true;
    }

private:
    bool m_IsOpen = true;
    std::string m_PanelName;
};


// public API

osc::PerfPanel::PerfPanel(std::string_view panelName) :
    m_Impl{std::make_unique<Impl>(std::move(panelName))}
{
}

osc::PerfPanel::PerfPanel(PerfPanel&&) noexcept = default;
osc::PerfPanel& osc::PerfPanel::operator=(PerfPanel&&) noexcept = default;
osc::PerfPanel::~PerfPanel() = default;

void osc::PerfPanel::open()
{
    m_Impl->open();
}

void osc::PerfPanel::close()
{
    m_Impl->close();
}

bool osc::PerfPanel::draw()
{
    return m_Impl->draw();
}

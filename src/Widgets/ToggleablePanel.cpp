#include "ToggleablePanel.hpp"

#include "src/Widgets/Panel.hpp"
#include "src/Widgets/ToggleablePanelFlags.hpp"
#include "src/Utils/CStringView.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

osc::ToggleablePanel::ToggleablePanel(
    std::string_view name_,
    std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
    ToggleablePanelFlags flags_) :

    m_Name{std::move(name_)},
    m_ConstructorFunc{std::move(constructorFunc_)},
    m_Flags{std::move(flags_)},
    m_Instance{std::nullopt}
{
}

osc::ToggleablePanel::ToggleablePanel(ToggleablePanel&&) noexcept = default;
osc::ToggleablePanel& osc::ToggleablePanel::operator=(ToggleablePanel&&) noexcept = default;
osc::ToggleablePanel::~ToggleablePanel() noexcept = default;

osc::CStringView osc::ToggleablePanel::getName() const
{
    return m_Name;
}

bool osc::ToggleablePanel::isEnabledByDefault() const
{
    return m_Flags & ToggleablePanelFlags_IsEnabledByDefault;
}

bool osc::ToggleablePanel::isActivated() const
{
    return m_Instance.has_value();
}

void osc::ToggleablePanel::activate()
{
    if (!m_Instance)
    {
        m_Instance = m_ConstructorFunc(m_Name);
    }
}

void osc::ToggleablePanel::deactivate()
{
    m_Instance.reset();
}

void osc::ToggleablePanel::toggleActivation()
{
    if (m_Instance && (*m_Instance)->isOpen())
    {
        m_Instance.reset();
    }
    else
    {
        m_Instance = m_ConstructorFunc(m_Name);
        (*m_Instance)->open();
    }
}

void osc::ToggleablePanel::draw()
{
    if (m_Instance)
    {
        (*m_Instance)->draw();
    }
}

// clear any instance data if the panel has been closed
void osc::ToggleablePanel::garbageCollect()
{
    if (m_Instance && !(*m_Instance)->isOpen())
    {
        m_Instance.reset();
    }
}

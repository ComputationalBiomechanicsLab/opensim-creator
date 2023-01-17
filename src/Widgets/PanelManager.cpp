#include "PanelManager.hpp"

#include "src/Widgets/Panel.hpp"
#include "src/Widgets/ToggleablePanelFlags.hpp"
#include "src/Utils/CStringView.hpp"

#include <nonstd/span.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    // a panel that the user may be able to toggle at runtime
    class ToggleablePanel final {
    public:
        ToggleablePanel(
            std::string_view name_,
            std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
            osc::ToggleablePanelFlags flags_ = osc::ToggleablePanelFlags_Default) :

            m_Name{std::move(name_)},
            m_ConstructorFunc{std::move(constructorFunc_)},
            m_Flags{std::move(flags_)},
            m_Instance{std::nullopt}
        {
        }

        osc::CStringView getName() const
        {
            return m_Name;
        }

        bool isEnabledByDefault() const
        {
            return m_Flags & osc::ToggleablePanelFlags_IsEnabledByDefault;
        }

        bool isActivated() const
        {
            return m_Instance.has_value();
        }

        void activate()
        {
            if (!m_Instance)
            {
                m_Instance = m_ConstructorFunc(m_Name);
            }
        }

        void deactivate()
        {
            m_Instance.reset();
        }

        void toggleActivation()
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

        void draw()
        {
            if (m_Instance)
            {
                (*m_Instance)->draw();
            }
        }

        // clear any instance data if the panel has been closed
        void garbageCollect()
        {
            if (m_Instance && !(*m_Instance)->isOpen())
            {
                m_Instance.reset();
            }
        }

    private:
        std::string m_Name;
        std::function<std::shared_ptr<osc::Panel>(std::string_view)> m_ConstructorFunc;
        osc::ToggleablePanelFlags m_Flags;
        std::optional<std::shared_ptr<osc::Panel>> m_Instance;
    };
}

class osc::PanelManager::Impl final {
public:
    void registerPanel(
        std::string_view baseName,
        std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
        ToggleablePanelFlags flags_ = ToggleablePanelFlags_Default)
    {
        push_back(ToggleablePanel{baseName, constructorFunc_, flags_});
    }

    size_t getNumToggleablePanels() const
    {
        return m_Panels.size();
    }

    CStringView getToggleablePanelName(size_t i) const
    {
        return m_Panels.at(i).getName();
    }

    bool isToggleablePanelActivated(size_t i) const
    {
        return m_Panels.at(i).isActivated();
    }

    void setToggleablePanelActivated(size_t i, bool v)
    {
        ToggleablePanel& panel = m_Panels.at(i);
        if (panel.isActivated() != v)
        {
            panel.toggleActivation();
        }
    }

    void activateAllDefaultOpenPanels()
    {
        // initialize default-open tabs
        for (ToggleablePanel& panel : m_Panels)
        {
            if (panel.isEnabledByDefault())
            {
                panel.activate();
            }
        }
    }

    void garbageCollectDeactivatedPanels()
    {
        // garbage collect closed-panel instance data
        for (ToggleablePanel& panel : m_Panels)
        {
            panel.garbageCollect();
        }
    }

    void drawAllActivatedPanels()
    {
        for (ToggleablePanel& panel : m_Panels)
        {
            if (panel.isActivated())
            {
                panel.draw();
            }
        }
    }

private:
    void push_back(ToggleablePanel&& panel)
    {
        m_Panels.push_back(std::move(panel));
    }

    std::vector<ToggleablePanel> m_Panels;
};


// public API (PIMPL)

osc::PanelManager::PanelManager() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::PanelManager::PanelManager(PanelManager&&) noexcept = default;
osc::PanelManager& osc::PanelManager::operator=(PanelManager&&) noexcept = default;
osc::PanelManager::~PanelManager() noexcept = default;

void osc::PanelManager::registerPanel(
    std::string_view baseName,
    std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
    ToggleablePanelFlags flags_)
{
    m_Impl->registerPanel(std::move(baseName), std::move(constructorFunc_), std::move(flags_));
}

size_t osc::PanelManager::getNumToggleablePanels() const
{
    return m_Impl->getNumToggleablePanels();
}

osc::CStringView osc::PanelManager::getToggleablePanelName(size_t i) const
{
    return m_Impl->getToggleablePanelName(i);
}

bool osc::PanelManager::isToggleablePanelActivated(size_t i) const
{
    return m_Impl->isToggleablePanelActivated(i);
}

void osc::PanelManager::setToggleablePanelActivated(size_t i, bool v)
{
    m_Impl->setToggleablePanelActivated(i, v);
}

void osc::PanelManager::activateAllDefaultOpenPanels()
{
    m_Impl->activateAllDefaultOpenPanels();
}

void osc::PanelManager::garbageCollectDeactivatedPanels()
{
    m_Impl->garbageCollectDeactivatedPanels();
}

void osc::PanelManager::drawAllActivatedPanels()
{
    m_Impl->drawAllActivatedPanels();
}

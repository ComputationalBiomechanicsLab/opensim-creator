#include "PanelManager.hpp"

#include "oscar/Panels/Panel.hpp"
#include "oscar/Panels/ToggleablePanelFlags.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    // an that the user can toggle in-place at runtime
    class ToggleablePanel final {
    public:
        ToggleablePanel(
            std::string_view name_,
            std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
            osc::ToggleablePanelFlags flags_) :

            m_Name{name_},
            m_ConstructorFunc{std::move(constructorFunc_)},
            m_Flags{flags_},
            m_Instance{std::nullopt}
        {
        }

        osc::Panel* updPtrOrNull()
        {
            return m_Instance ? m_Instance->get() : nullptr;
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

    class DynamicPanel final {
    public:
        DynamicPanel(
            std::string_view baseName,
            size_t instanceNumber,
            std::shared_ptr<osc::Panel> instance) :

            m_SpawnerID{std::hash<std::string_view>{}(baseName)},
            m_InstanceNumber{instanceNumber},
            m_Instance{std::move(instance)}
        {
            m_Instance->open();
        }

        osc::Panel* updPtrOrNull()
        {
            return m_Instance.get();
        }

        size_t getSpawnablePanelID() const
        {
            return m_SpawnerID;
        }

        size_t getInstanceNumber() const
        {
            return m_InstanceNumber;
        }

        osc::CStringView getName() const
        {
            return m_Instance->getName();
        }

        bool isOpen() const
        {
            return m_Instance->isOpen();
        }

        void draw()
        {
            m_Instance->draw();
        }

    private:
        size_t m_SpawnerID;
        size_t m_InstanceNumber;
        std::shared_ptr<osc::Panel> m_Instance;
    };

    // declaration for a panel that can spawn new dyanmic panels (above)
    class SpawnablePanel final {
    public:
        SpawnablePanel(
            std::string_view baseName_,
            std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
            size_t numInitiallyOpenedPanels_) :

            m_BaseName{baseName_},
            m_ConstructorFunc{std::move(constructorFunc_)},
            m_NumInitiallyOpenedPanels{numInitiallyOpenedPanels_}
        {
        }

        size_t getID() const
        {
            return std::hash<std::string_view>{}(m_BaseName);
        }

        osc::CStringView getBaseName() const
        {
            return m_BaseName;
        }

        DynamicPanel spawnDynamicPanel(size_t ithInstance, std::string_view panelName)
        {
            return DynamicPanel
            {
                m_BaseName,         // so outside code knows which spawnable panel made it
                ithInstance,        // so outside code can reassign `i` later based on open/close logic
                m_ConstructorFunc(panelName),
            };
        }

        size_t getNumInitiallyOpenedPanels() const
        {
            return m_NumInitiallyOpenedPanels;
        }

    private:
        std::string m_BaseName;
        std::function<std::shared_ptr<osc::Panel>(std::string_view)> m_ConstructorFunc;
        size_t m_NumInitiallyOpenedPanels;
    };
}

class osc::PanelManager::Impl final {
public:

    void registerToggleablePanel(
        std::string_view baseName,
        std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
        ToggleablePanelFlags flags_)
    {
        m_ToggleablePanels.emplace_back(baseName, std::move(constructorFunc_), flags_);
    }

    void registerSpawnablePanel(
        std::string_view baseName,
        std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
        size_t numInitiallyOpenedPanels)
    {
        m_SpawnablePanels.emplace_back(baseName, constructorFunc_, numInitiallyOpenedPanels);
    }

    Panel* tryUpdPanelByName(std::string_view name)
    {
        for (ToggleablePanel& panel : m_ToggleablePanels)
        {
            if (Panel* p = panel.updPtrOrNull(); p && p->getName() == name)
            {
                return p;
            }
        }

        for (DynamicPanel& panel : m_DynamicPanels)
        {
            if (Panel* p = panel.updPtrOrNull(); p && p->getName() == name)
            {
                return p;
            }
        }

        return nullptr;
    }

    size_t getNumToggleablePanels() const
    {
        return m_ToggleablePanels.size();
    }

    CStringView getToggleablePanelName(size_t i) const
    {
        return m_ToggleablePanels.at(i).getName();
    }

    bool isToggleablePanelActivated(size_t i) const
    {
        return m_ToggleablePanels.at(i).isActivated();
    }

    void setToggleablePanelActivated(size_t i, bool v)
    {
        ToggleablePanel& panel = m_ToggleablePanels.at(i);
        if (panel.isActivated() != v)
        {
            panel.toggleActivation();
        }
    }

    void setToggleablePanelActivated(std::string_view panelName, bool v)
    {
        for (ToggleablePanel& panel : m_ToggleablePanels)
        {
            if (panel.getName() == panelName)
            {
                if (v)
                {
                    panel.activate();
                }
                else
                {
                    panel.deactivate();
                }
            }
        }
    }

    void onMount()
    {
        if (!m_FirstMount)
        {
            return;  // already mounted once
        }

        // initialize default-open tabs
        for (ToggleablePanel& panel : m_ToggleablePanels)
        {
            if (panel.isEnabledByDefault())
            {
                panel.activate();
            }
        }

        // initialize dynamic tabs that have some "initial" number
        // of spawned tabs
        for (size_t iPanel = 0; iPanel < m_SpawnablePanels.size(); ++iPanel)
        {
            SpawnablePanel& panel = m_SpawnablePanels[iPanel];
            for (size_t i = 0; i < panel.getNumInitiallyOpenedPanels(); ++i)
            {
                createDynamicPanel(iPanel);
            }
        }

        m_FirstMount = false;
    }

    void onUnmount()
    {
        // noop: we only mount the panels once and never unmount them
    }

    void onTick()
    {
        // garbage collect closed-panel instance data
        for (ToggleablePanel& panel : m_ToggleablePanels)
        {
            panel.garbageCollect();
        }

        for (size_t i = 0; i < m_DynamicPanels.size(); ++i)
        {
            if (!m_DynamicPanels.at(i).isOpen())
            {
                m_DynamicPanels.erase(m_DynamicPanels.begin() + i);
                --i;
            }
        }
    }

    void onDraw()
    {
        for (ToggleablePanel& panel : m_ToggleablePanels)
        {
            if (panel.isActivated())
            {
                panel.draw();
            }
        }

        for (DynamicPanel& panel : m_DynamicPanels)
        {
            panel.draw();
        }
    }

    size_t getNumDynamicPanels() const
    {
        return m_DynamicPanels.size();
    }

    CStringView getDynamicPanelName(size_t i) const
    {
        return m_DynamicPanels.at(i).getName();
    }

    void deactivateDynamicPanel(size_t i)
    {
        if (i < m_DynamicPanels.size())
        {
            m_DynamicPanels.erase(m_DynamicPanels.begin() + i);
        }
    }

    size_t getNumSpawnablePanels() const
    {
        return m_SpawnablePanels.size();
    }

    CStringView getSpawnablePanelBaseName(size_t i) const
    {
        return m_SpawnablePanels.at(i).getBaseName();
    }

    void createDynamicPanel(size_t i)
    {
        SpawnablePanel& spawnable = m_SpawnablePanels.at(i);
        size_t const ithInstance = calcDynamicPanelInstanceNumber(spawnable.getID());
        std::string const panelName = calcPanelName(spawnable.getBaseName(), ithInstance);
        DynamicPanel p = spawnable.spawnDynamicPanel(ithInstance, panelName);
        pushDynamicPanel(std::move(p));
    }

    std::string computeSuggestedDynamicPanelName(std::string_view baseName)
    {
        size_t const ithInstance = calcDynamicPanelInstanceNumber(std::hash<std::string_view>{}(baseName));
        return calcPanelName(baseName, ithInstance);
    }

    void pushDynamicPanel(std::string_view baseName, std::shared_ptr<Panel> panel)
    {
        size_t const ithInstance = calcDynamicPanelInstanceNumber(std::hash<std::string_view>{}(baseName));
        pushDynamicPanel(DynamicPanel{baseName, ithInstance, std::move(panel)});
    }

private:
    size_t calcDynamicPanelInstanceNumber(size_t spawnableID)
    {
        // compute instance index such that it's the lowest non-colliding
        // number with the same spawnable panel

        std::vector<bool> used(m_DynamicPanels.size());
        for (DynamicPanel& panel : m_DynamicPanels)
        {
            if (panel.getSpawnablePanelID() == spawnableID)
            {
                size_t instanceNumber = panel.getInstanceNumber();
                used.resize(std::max(instanceNumber, used.size()));
                used[instanceNumber] = true;
            }
        }
        return std::distance(used.begin(), std::find(used.begin(), used.end(), false));
    }

    std::string calcPanelName(std::string_view baseName, size_t ithInstance)
    {
        std::stringstream ss;
        ss << baseName << ithInstance;
        return std::move(ss).str();
    }

    void pushDynamicPanel(DynamicPanel p)
    {
        m_DynamicPanels.push_back(std::move(p));

        // re-sort so that panels are clustered correctly
        std::sort(
            m_DynamicPanels.begin(),
            m_DynamicPanels.end(),
            [](DynamicPanel const& a, DynamicPanel const& b)
            {
                if (a.getSpawnablePanelID() != b.getSpawnablePanelID())
                {
                    return a.getSpawnablePanelID() < b.getSpawnablePanelID();
                }
                else
                {
                    return a.getInstanceNumber() < b.getInstanceNumber();
                }
            });
    }

    std::vector<ToggleablePanel> m_ToggleablePanels;
    std::vector<DynamicPanel> m_DynamicPanels;
    std::vector<SpawnablePanel> m_SpawnablePanels;
    bool m_FirstMount = true;
};


// public API (PIMPL)

osc::PanelManager::PanelManager() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::PanelManager::PanelManager(PanelManager&&) noexcept = default;
osc::PanelManager& osc::PanelManager::operator=(PanelManager&&) noexcept = default;
osc::PanelManager::~PanelManager() noexcept = default;

void osc::PanelManager::registerToggleablePanel(
    std::string_view baseName,
    std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
    ToggleablePanelFlags flags_)
{
    m_Impl->registerToggleablePanel(baseName, std::move(constructorFunc_), flags_);
}

void osc::PanelManager::registerSpawnablePanel(
    std::string_view baseName,
    std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
    size_t numInitiallyOpenedPanels)
{
    m_Impl->registerSpawnablePanel(baseName, std::move(constructorFunc_), numInitiallyOpenedPanels);
}

osc::Panel* osc::PanelManager::tryUpdPanelByName(std::string_view name)
{
    return m_Impl->tryUpdPanelByName(name);
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

void osc::PanelManager::setToggleablePanelActivated(std::string_view panelName, bool v)
{
    m_Impl->setToggleablePanelActivated(panelName, v);
}

void osc::PanelManager::onMount()
{
    m_Impl->onMount();
}

void osc::PanelManager::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::PanelManager::onTick()
{
    m_Impl->onTick();
}

void osc::PanelManager::onDraw()
{
    m_Impl->onDraw();
}

size_t osc::PanelManager::getNumDynamicPanels() const
{
    return m_Impl->getNumDynamicPanels();
}

osc::CStringView osc::PanelManager::getDynamicPanelName(size_t i) const
{
    return m_Impl->getDynamicPanelName(i);
}

void osc::PanelManager::deactivateDynamicPanel(size_t i)
{
    m_Impl->deactivateDynamicPanel(i);
}

size_t osc::PanelManager::getNumSpawnablePanels() const
{
    return m_Impl->getNumSpawnablePanels();
}

osc::CStringView osc::PanelManager::getSpawnablePanelBaseName(size_t i) const
{
    return m_Impl->getSpawnablePanelBaseName(i);
}

void osc::PanelManager::createDynamicPanel(size_t i)
{
    m_Impl->createDynamicPanel(i);
}

std::string osc::PanelManager::computeSuggestedDynamicPanelName(std::string_view baseName)
{
    return m_Impl->computeSuggestedDynamicPanelName(baseName);
}

void osc::PanelManager::pushDynamicPanel(std::string_view baseName, std::shared_ptr<Panel> panel)
{
    m_Impl->pushDynamicPanel(baseName, std::move(panel));
}

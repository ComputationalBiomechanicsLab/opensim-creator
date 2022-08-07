#include "TabRegistry.hpp"

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/SynchronizedValue.hpp"

// registered tabs
#include "src/Tabs/Experiments/ImGuiDemoTab.hpp"
#include "src/Tabs/Experiments/ImPlotDemoTab.hpp"
#include "src/Tabs/Experiments/PreviewExperimentalDataTab.hpp"
#include "src/Tabs/Experiments/RendererBasicLightingTab.hpp"
#include "src/Tabs/Experiments/RendererBlendingTab.hpp"
#include "src/Tabs/Experiments/RendererCoordinateSystemsTab.hpp"
#include "src/Tabs/Experiments/RendererFramebuffersTab.hpp"
#include "src/Tabs/Experiments/RendererGeometryShaderTab.hpp"
#include "src/Tabs/Experiments/RendererHelloTriangleTab.hpp"
#include "src/Tabs/Experiments/RendererLightingMapsTab.hpp"
#include "src/Tabs/Experiments/RendererMultipleLightsTab.hpp"
#include "src/Tabs/Experiments/RendererOpenSimTab.hpp"
#include "src/Tabs/Experiments/RendererSDFTab.hpp"
#include "src/Tabs/Experiments/RendererTexturingTab.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

class osc::TabRegistryEntry::Impl final {
public:
    Impl(CStringView name_, std::unique_ptr<Tab>(*ctor_)(TabHost*)) :
        m_Name{std::move(name_)},
        m_Constructor{std::move(ctor_)}
    {
    }

    CStringView getName() const
    {
        return m_Name;
    }

    std::unique_ptr<Tab> createTab(TabHost* host) const
    {
        return m_Constructor(std::move(host));
    }

private:
    std::string m_Name;
    std::unique_ptr<Tab>(*m_Constructor)(TabHost*);
};

// init + storage for the global tab table
namespace
{
    template<typename T>
    std::unique_ptr<osc::Tab> TabConstructor(osc::TabHost* h)
    {
        return std::make_unique<T>(std::move(h));
    }

    osc::SynchronizedValue<std::vector<osc::TabRegistryEntry>> InitDefaultTabs()
    {
        osc::SynchronizedValue<std::vector<osc::TabRegistryEntry>> rv;

        auto lock = rv.lock();
        lock->emplace_back("OpenSim/PreviewExperimentalData", TabConstructor<osc::PreviewExperimentalDataTab>);
        lock->emplace_back("Renderer/BasicLighting", TabConstructor<osc::RendererBasicLightingTab>);
        lock->emplace_back("Renderer/Blending", TabConstructor<osc::RendererBlendingTab>);
        lock->emplace_back("Renderer/CoordinateSystems", TabConstructor<osc::RendererCoordinateSystemsTab>);
        lock->emplace_back("Renderer/Framebuffers", TabConstructor<osc::RendererFramebuffersTab>);
        lock->emplace_back("Renderer/GeometryShader", TabConstructor<osc::RendererGeometryShaderTab>);
        lock->emplace_back("Renderer/HelloTriangle", TabConstructor<osc::RendererHelloTriangleTab>);
        lock->emplace_back("Renderer/LightingMaps", TabConstructor<osc::RendererLightingMapsTab>);
        lock->emplace_back("Renderer/MultipleLights", TabConstructor<osc::RendererMultipleLightsTab>);
        lock->emplace_back("Renderer/OpenSimModel", TabConstructor<osc::RendererOpenSimTab>);
        lock->emplace_back("Renderer/Texturing", TabConstructor<osc::RendererTexturingTab>);
        lock->emplace_back("Renderer/SDFTab", TabConstructor<osc::RendererSDFTab>);
        lock->emplace_back("Demos/ImGui", TabConstructor<osc::ImGuiDemoTab>);
        lock->emplace_back("Demos/ImPlot", TabConstructor<osc::ImPlotDemoTab>);

        std::sort(lock->begin(), lock->end());

        return rv;
    }

    osc::SynchronizedValueGuard<std::vector<osc::TabRegistryEntry>> GetRegisteredTabsTable()
    {
        static osc::SynchronizedValue<std::vector<osc::TabRegistryEntry>> g_Entries{InitDefaultTabs()};
        return g_Entries.lock();
    }
}

// public API

osc::TabRegistryEntry::TabRegistryEntry(CStringView name_, std::unique_ptr<Tab>(*ctor_)(TabHost*)) :
    m_Impl{std::make_shared<Impl>(std::move(name_), std::move(ctor_))}
{
}

osc::TabRegistryEntry::TabRegistryEntry(TabRegistryEntry const&) = default;
osc::TabRegistryEntry::TabRegistryEntry(TabRegistryEntry&&) noexcept = default;
osc::TabRegistryEntry& osc::TabRegistryEntry::operator=(TabRegistryEntry const&) = default;
osc::TabRegistryEntry& osc::TabRegistryEntry::operator=(TabRegistryEntry&&) = default;
osc::TabRegistryEntry::~TabRegistryEntry() noexcept = default;

osc::CStringView osc::TabRegistryEntry::getName() const
{
    return m_Impl->getName();
}

std::unique_ptr<osc::Tab> osc::TabRegistryEntry::createTab(TabHost* host) const
{
    return m_Impl->createTab(std::move(host));
}

bool osc::operator<(TabRegistryEntry const& a, TabRegistryEntry const& b)
{
    return a.getName() < b.getName();
}

int osc::GetNumRegisteredTabs()
{
    auto lock = GetRegisteredTabsTable();
    return static_cast<int>(lock->size());
}

osc::TabRegistryEntry osc::GetRegisteredTab(int i)
{
    return GetRegisteredTabsTable()->at(static_cast<size_t>(i));
}

std::optional<osc::TabRegistryEntry> osc::GetRegisteredTabByName(std::string_view s)
{
    auto lock = GetRegisteredTabsTable();
    auto it = std::find_if(lock->begin(), lock->end(), [&s](osc::TabRegistryEntry const& e)
    {
        return e.getName() == s;
    });
    return it != lock->end() ? *it : std::optional<osc::TabRegistryEntry>{};
}

bool osc::RegisterTab(CStringView name, std::unique_ptr<Tab>(*ctor_)(TabHost*))
{
    auto lock = GetRegisteredTabsTable();
    lock->emplace_back(std::move(name), std::move(ctor_));
    std::sort(lock->begin(), lock->end());
    return true;
}
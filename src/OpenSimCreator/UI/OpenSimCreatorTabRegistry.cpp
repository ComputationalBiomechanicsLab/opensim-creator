#include "OpenSimCreatorTabRegistry.hpp"

#include <OpenSimCreator/UI/Experimental/MeshHittestTab.hpp>
#include <OpenSimCreator/UI/Experimental/RendererGeometryShaderTab.hpp>
#include <OpenSimCreator/UI/Experimental/TPS2DTab.hpp>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionTab.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTab.hpp>
#include <OpenSimCreator/UI/ModelWarper/ModelWarperTab.hpp>

#include <oscar/UI/Tabs/TabRegistry.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <concepts>
#include <memory>

using namespace osc;

namespace
{
    template<std::derived_from<ITab> TabType>
    void RegisterTab(TabRegistry& registry)
    {
        TabRegistryEntry entry
        {
            TabType::id(),
            [](ParentPtr<ITabHost> const& h) { return std::make_unique<TabType>(h); },
        };
        registry.registerTab(entry);
    }
}

void osc::RegisterOpenSimCreatorTabs(TabRegistry& registry)
{
    RegisterTab<MeshHittestTab>(registry);
    RegisterTab<RendererGeometryShaderTab>(registry);
    RegisterTab<TPS2DTab>(registry);
    RegisterTab<MeshWarpingTab>(registry);
    RegisterTab<mow::ModelWarperTab>(registry);
    RegisterTab<FrameDefinitionTab>(registry);
}

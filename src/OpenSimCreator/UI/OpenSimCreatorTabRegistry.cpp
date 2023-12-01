#include "OpenSimCreatorTabRegistry.hpp"

#include <OpenSimCreator/UI/Experimental/MeshHittestTab.hpp>
#include <OpenSimCreator/UI/Experimental/RendererGeometryShaderTab.hpp>
#include <OpenSimCreator/UI/Experimental/TPS2DTab.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTab.hpp>
#include <OpenSimCreator/UI/FrameDefinitionTab.hpp>

#include <oscar/UI/Tabs/TabRegistry.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <memory>

namespace
{
    template<typename TabType>
    void RegisterTab(osc::TabRegistry& registry)
    {
        osc::TabRegistryEntry entry
        {
            TabType::id(),
            [](osc::ParentPtr<osc::TabHost> const& h) { return std::make_unique<TabType>(h); },
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
    RegisterTab<FrameDefinitionTab>(registry);
}

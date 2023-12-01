#include "LearnOpenGLTabRegistry.hpp"

#include <oscar_learnopengl/AdvancedLighting/LOGLBloomTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLDeferredShadingTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLGammaTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLHDRTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLNormalMappingTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLParallaxMappingTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLPointShadowsTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLShadowMappingTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLSSAOTab.hpp>
#include <oscar_learnopengl/AdvancedOpenGL/LOGLBlendingTab.hpp>
#include <oscar_learnopengl/AdvancedOpenGL/LOGLCubemapsTab.hpp>
#include <oscar_learnopengl/AdvancedOpenGL/LOGLFaceCullingTab.hpp>
#include <oscar_learnopengl/AdvancedOpenGL/LOGLFramebuffersTab.hpp>
#include <oscar_learnopengl/GettingStarted/LOGLCoordinateSystemsTab.hpp>
#include <oscar_learnopengl/GettingStarted/LOGLHelloTriangleTab.hpp>
#include <oscar_learnopengl/GettingStarted/LOGLTexturingTab.hpp>
#include <oscar_learnopengl/Lighting/LOGLBasicLightingTab.hpp>
#include <oscar_learnopengl/Lighting/LOGLLightingMapsTab.hpp>
#include <oscar_learnopengl/Lighting/LOGLMultipleLightsTab.hpp>
#include <oscar_learnopengl/PBR/LOGLPBRDiffuseIrradianceTab.hpp>
#include <oscar_learnopengl/PBR/LOGLPBRLightingTab.hpp>
#include <oscar_learnopengl/PBR/LOGLPBRLightingTexturedTab.hpp>
#include <oscar_learnopengl/PBR/LOGLPBRSpecularIrradianceTab.hpp>
#include <oscar_learnopengl/PBR/LOGLPBRSpecularIrradianceTexturedTab.hpp>

#include <oscar/UI/Tabs/TabRegistry.hpp>
#include <oscar/UI/Tabs/TabRegistryEntry.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <memory>

using osc::TabHost;
using osc::TabRegistry;
using osc::TabRegistryEntry;

namespace
{
    template<typename TabType>
    void RegisterTab(TabRegistry& registry)
    {
        TabRegistryEntry entry
        {
            TabType::id(),
            [](osc::ParentPtr<TabHost> const& h) { return std::make_unique<TabType>(h); },
        };
        registry.registerTab(entry);
    }
}

void osc::RegisterLearnOpenGLTabs(TabRegistry& registry)
{
    RegisterTab<LOGLBloomTab>(registry);
    RegisterTab<LOGLDeferredShadingTab>(registry);
    RegisterTab<LOGLGammaTab>(registry);
    RegisterTab<LOGLHDRTab>(registry);
    RegisterTab<LOGLNormalMappingTab>(registry);
    RegisterTab<LOGLParallaxMappingTab>(registry);
    RegisterTab<LOGLPointShadowsTab>(registry);
    RegisterTab<LOGLShadowMappingTab>(registry);
    RegisterTab<LOGLSSAOTab>(registry);

    RegisterTab<LOGLBlendingTab>(registry);
    RegisterTab<LOGLCubemapsTab>(registry);
    RegisterTab<LOGLFaceCullingTab>(registry);
    RegisterTab<LOGLFramebuffersTab>(registry);

    RegisterTab<LOGLCoordinateSystemsTab>(registry);
    RegisterTab<LOGLHelloTriangleTab>(registry);
    RegisterTab<LOGLTexturingTab>(registry);

    RegisterTab<LOGLBasicLightingTab>(registry);
    RegisterTab<LOGLLightingMapsTab>(registry);
    RegisterTab<LOGLMultipleLightsTab>(registry);

    RegisterTab<LOGLPBRDiffuseIrradianceTab>(registry);
    RegisterTab<LOGLPBRLightingTab>(registry);
    RegisterTab<LOGLPBRLightingTexturedTab>(registry);
    RegisterTab<LOGLPBRSpecularIrradianceTab>(registry);
    RegisterTab<LOGLPBRSpecularIrradianceTexturedTab>(registry);
}

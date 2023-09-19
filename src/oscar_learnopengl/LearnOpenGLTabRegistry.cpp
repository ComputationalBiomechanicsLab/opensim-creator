#include "LearnOpenGLTabRegistry.hpp"

#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLBloomTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLDeferredShadingTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLGammaTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLHDRTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLNormalMappingTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLParallaxMappingTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLPointShadowsTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLShadowMappingTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedLighting/LOGLSSAOTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedOpenGL/LOGLBlendingTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedOpenGL/LOGLCubemapsTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedOpenGL/LOGLFaceCullingTab.hpp>
#include <oscar_learnopengl/Tabs/AdvancedOpenGL/LOGLFramebuffersTab.hpp>
#include <oscar_learnopengl/Tabs/GettingStarted/LOGLCoordinateSystemsTab.hpp>
#include <oscar_learnopengl/Tabs/GettingStarted/LOGLHelloTriangleTab.hpp>
#include <oscar_learnopengl/Tabs/GettingStarted/LOGLTexturingTab.hpp>
#include <oscar_learnopengl/Tabs/Lighting/LOGLBasicLightingTab.hpp>
#include <oscar_learnopengl/Tabs/Lighting/LOGLLightingMapsTab.hpp>
#include <oscar_learnopengl/Tabs/Lighting/LOGLMultipleLightsTab.hpp>
#include <oscar_learnopengl/Tabs/PBR/LOGLPBRDiffuseIrradianceTab.hpp>
#include <oscar_learnopengl/Tabs/PBR/LOGLPBRLightingTab.hpp>
#include <oscar_learnopengl/Tabs/PBR/LOGLPBRLightingTexturedTab.hpp>
#include <oscar_learnopengl/Tabs/PBR/LOGLPBRSpecularIrradianceTab.hpp>
#include <oscar_learnopengl/Tabs/PBR/LOGLPBRSpecularIrradianceTexturedTab.hpp>

#include <oscar/Tabs/TabRegistry.hpp>
#include <oscar/Tabs/TabRegistryEntry.hpp>
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

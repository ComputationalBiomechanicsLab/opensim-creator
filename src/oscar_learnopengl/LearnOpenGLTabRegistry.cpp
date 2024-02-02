#include "LearnOpenGLTabRegistry.hpp"

#include <oscar_learnopengl/AdvancedLighting/LOGLBloomTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLDeferredShadingTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLGammaTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLHDRTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLNormalMappingTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLParallaxMappingTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLPointShadowsTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLSSAOTab.hpp>
#include <oscar_learnopengl/AdvancedLighting/LOGLShadowMappingTab.hpp>
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
#include <oscar/Utils/Typelist.hpp>

void osc::RegisterLearnOpenGLTabs(TabRegistry& registry)
{
    using LearnOpenGLTabs = Typelist<
        LOGLBloomTab,
        LOGLDeferredShadingTab,
        LOGLGammaTab,
        LOGLHDRTab,
        LOGLNormalMappingTab,
        LOGLParallaxMappingTab,
        LOGLPointShadowsTab,
        LOGLShadowMappingTab,
        LOGLSSAOTab,

        LOGLBlendingTab,
        LOGLCubemapsTab,
        LOGLFaceCullingTab,
        LOGLFramebuffersTab,

        LOGLCoordinateSystemsTab,
        LOGLHelloTriangleTab,
        LOGLTexturingTab,

        LOGLBasicLightingTab,
        LOGLLightingMapsTab,
        LOGLMultipleLightsTab,

        LOGLPBRDiffuseIrradianceTab,
        LOGLPBRLightingTab,
        LOGLPBRLightingTexturedTab,
        LOGLPBRSpecularIrradianceTab,
        LOGLPBRSpecularIrradianceTexturedTab
    >;

    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.registerTab<Tabs>(), ...);
    }(LearnOpenGLTabs{});
}

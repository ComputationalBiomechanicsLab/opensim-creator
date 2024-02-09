#pragma once

#include <oscar_learnopengl/AdvancedLighting/LOGLBloomTab.h>
#include <oscar_learnopengl/AdvancedLighting/LOGLDeferredShadingTab.h>
#include <oscar_learnopengl/AdvancedLighting/LOGLGammaTab.h>
#include <oscar_learnopengl/AdvancedLighting/LOGLHDRTab.h>
#include <oscar_learnopengl/AdvancedLighting/LOGLNormalMappingTab.h>
#include <oscar_learnopengl/AdvancedLighting/LOGLParallaxMappingTab.h>
#include <oscar_learnopengl/AdvancedLighting/LOGLPointShadowsTab.h>
#include <oscar_learnopengl/AdvancedLighting/LOGLSSAOTab.h>
#include <oscar_learnopengl/AdvancedLighting/LOGLShadowMappingTab.h>
#include <oscar_learnopengl/AdvancedOpenGL/LOGLBlendingTab.h>
#include <oscar_learnopengl/AdvancedOpenGL/LOGLCubemapsTab.h>
#include <oscar_learnopengl/AdvancedOpenGL/LOGLFaceCullingTab.h>
#include <oscar_learnopengl/AdvancedOpenGL/LOGLFramebuffersTab.h>
#include <oscar_learnopengl/GettingStarted/LOGLCoordinateSystemsTab.h>
#include <oscar_learnopengl/GettingStarted/LOGLHelloTriangleTab.h>
#include <oscar_learnopengl/GettingStarted/LOGLTexturingTab.h>
#include <oscar_learnopengl/Lighting/LOGLBasicLightingTab.h>
#include <oscar_learnopengl/Lighting/LOGLLightingMapsTab.h>
#include <oscar_learnopengl/Lighting/LOGLMultipleLightsTab.h>
#include <oscar_learnopengl/PBR/LOGLPBRDiffuseIrradianceTab.h>
#include <oscar_learnopengl/PBR/LOGLPBRLightingTab.h>
#include <oscar_learnopengl/PBR/LOGLPBRLightingTexturedTab.h>
#include <oscar_learnopengl/PBR/LOGLPBRSpecularIrradianceTab.h>
#include <oscar_learnopengl/PBR/LOGLPBRSpecularIrradianceTexturedTab.h>

#include <oscar/Utils/Typelist.h>

namespace osc {

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
}

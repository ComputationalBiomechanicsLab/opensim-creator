#pragma once

#include <oscar_learnopengl/AdvancedLighting.h>
#include <oscar_learnopengl/AdvancedOpenGL.h>
#include <oscar_learnopengl/GettingStarted.h>
#include <oscar_learnopengl/Lighting.h>
#include <oscar_learnopengl/PBR.h>

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

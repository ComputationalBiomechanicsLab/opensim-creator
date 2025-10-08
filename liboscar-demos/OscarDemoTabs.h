#pragma once

#include <liboscar-demos/bookofshaders/BookOfShadersTab.h>

#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLBloomTab.h>
#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLDeferredShadingTab.h>
#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLGammaTab.h>
#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLHDRTab.h>
#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLNormalMappingTab.h>
#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLParallaxMappingTab.h>
#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLPointShadowsTab.h>
#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLShadowMappingTab.h>
#include <liboscar-demos/learnopengl/AdvancedLighting/LOGLSSAOTab.h>
#include <liboscar-demos/learnopengl/AdvancedOpenGL/LOGLBlendingTab.h>
#include <liboscar-demos/learnopengl/AdvancedOpenGL/LOGLCubemapsTab.h>
#include <liboscar-demos/learnopengl/AdvancedOpenGL/LOGLFaceCullingTab.h>
#include <liboscar-demos/learnopengl/AdvancedOpenGL/LOGLFramebuffersTab.h>
#include <liboscar-demos/learnopengl/GettingStarted/LOGLCoordinateSystemsTab.h>
#include <liboscar-demos/learnopengl/GettingStarted/LOGLHelloTriangleTab.h>
#include <liboscar-demos/learnopengl/GettingStarted/LOGLTexturingTab.h>
#include <liboscar-demos/learnopengl/Guest/LOGLCSMTab.h>
#include <liboscar-demos/learnopengl/Lighting/LOGLBasicLightingTab.h>
#include <liboscar-demos/learnopengl/Lighting/LOGLLightingMapsTab.h>
#include <liboscar-demos/learnopengl/Lighting/LOGLMultipleLightsTab.h>
#include <liboscar-demos/learnopengl/PBR/LOGLPBRDiffuseIrradianceTab.h>
#include <liboscar-demos/learnopengl/PBR/LOGLPBRLightingTab.h>
#include <liboscar-demos/learnopengl/PBR/LOGLPBRLightingTexturedTab.h>
#include <liboscar-demos/learnopengl/PBR/LOGLPBRSpecularIrradianceTab.h>
#include <liboscar-demos/learnopengl/PBR/LOGLPBRSpecularIrradianceTexturedTab.h>

#include <liboscar-demos/CustomWidgetsTab.h>
#include <liboscar-demos/DrawingTestTab.h>
#include <liboscar-demos/FrustumCullingTab.h>
#include <liboscar-demos/HittestTab.h>
#include <liboscar-demos/ImGuiDemoTab.h>
#include <liboscar-demos/ImGuizmoDemoTab.h>
#include <liboscar-demos/ImPlotDemoTab.h>
#include <liboscar-demos/MandelbrotTab.h>
#include <liboscar-demos/MeshGenTestTab.h>
#include <liboscar-demos/SubMeshTab.h>

#include <liboscar/Utils/Typelist.h>

namespace osc
{
    using OscarDemoTabs = Typelist<
        BookOfShadersTab,

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

        LOGLCSMTab,

        LOGLBasicLightingTab,
        LOGLLightingMapsTab,
        LOGLMultipleLightsTab,

        LOGLPBRDiffuseIrradianceTab,
        LOGLPBRLightingTab,
        LOGLPBRLightingTexturedTab,
        LOGLPBRSpecularIrradianceTab,
        LOGLPBRSpecularIrradianceTexturedTab,

        CustomWidgetsTab,
        DrawingTestTab,
        FrustumCullingTab,
        HittestTab,
        ImGuiDemoTab,
        ImPlotDemoTab,
        ImGuizmoDemoTab,
        MandelbrotTab,
        MeshGenTestTab,
        SubMeshTab
    >;
}

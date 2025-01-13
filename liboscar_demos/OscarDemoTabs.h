#pragma once

#include <liboscar_demos/bookofshaders/BookOfShadersTab.h>

#include <liboscar_demos/learnopengl/AdvancedLighting.h>
#include <liboscar_demos/learnopengl/AdvancedOpenGL.h>
#include <liboscar_demos/learnopengl/GettingStarted.h>
#include <liboscar_demos/learnopengl/Guest.h>
#include <liboscar_demos/learnopengl/Lighting.h>
#include <liboscar_demos/learnopengl/PBR.h>

#include <liboscar_demos/CustomWidgetsTab.h>
#include <liboscar_demos/FrustumCullingTab.h>
#include <liboscar_demos/HittestTab.h>
#include <liboscar_demos/ImGuiDemoTab.h>
#include <liboscar_demos/ImGuizmoDemoTab.h>
#include <liboscar_demos/ImPlotDemoTab.h>
#include <liboscar_demos/MandelbrotTab.h>
#include <liboscar_demos/MeshGenTestTab.h>
#include <liboscar_demos/SubMeshTab.h>

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

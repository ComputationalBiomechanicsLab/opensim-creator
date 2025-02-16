#pragma once

#include <liboscar-demos/bookofshaders/BookOfShadersTab.h>

#include <liboscar-demos/learnopengl/AdvancedLighting.h>
#include <liboscar-demos/learnopengl/AdvancedOpenGL.h>
#include <liboscar-demos/learnopengl/GettingStarted.h>
#include <liboscar-demos/learnopengl/Guest.h>
#include <liboscar-demos/learnopengl/Lighting.h>
#include <liboscar-demos/learnopengl/PBR.h>

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

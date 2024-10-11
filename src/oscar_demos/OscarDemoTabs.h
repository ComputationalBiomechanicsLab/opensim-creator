#pragma once

#include <oscar_demos/bookofshaders/BookOfShadersTab.h>

#include <oscar_demos/learnopengl/AdvancedLighting.h>
#include <oscar_demos/learnopengl/AdvancedOpenGL.h>
#include <oscar_demos/learnopengl/GettingStarted.h>
#include <oscar_demos/learnopengl/Guest.h>
#include <oscar_demos/learnopengl/Lighting.h>
#include <oscar_demos/learnopengl/PBR.h>

#include <oscar_demos/CustomWidgetsTab.h>
#include <oscar_demos/FrustrumCullingTab.h>
#include <oscar_demos/HittestTab.h>
#include <oscar_demos/ImGuiDemoTab.h>
#include <oscar_demos/ImGuizmoDemoTab.h>
#include <oscar_demos/ImPlotDemoTab.h>
#include <oscar_demos/MandelbrotTab.h>
#include <oscar_demos/MeshGenTestTab.h>
#include <oscar_demos/SubMeshTab.h>

#include <oscar/Utils/Typelist.h>

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
        FrustrumCullingTab,
        HittestTab,
        ImGuiDemoTab,
        ImPlotDemoTab,
        ImGuizmoDemoTab,
        MandelbrotTab,
        MeshGenTestTab,
        SubMeshTab
    >;
}

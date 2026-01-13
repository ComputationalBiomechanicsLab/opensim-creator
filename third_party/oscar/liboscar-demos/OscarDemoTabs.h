#pragma once

#include <liboscar-demos/bookofshaders/BookOfShadersTab.h>

#include <liboscar-demos/learnopengl/advanced_lighting/LOGLBloomTab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/LOGLDeferredShadingTab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/LOGLGammaTab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/LOGLHDRTab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/LOGLNormalMappingTab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/LOGLParallaxMappingTab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/LOGLPointShadowsTab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/LOGLShadowMappingTab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/LOGLSSAOTab.h>
#include <liboscar-demos/learnopengl/advanced_opengl/LOGLBlendingTab.h>
#include <liboscar-demos/learnopengl/advanced_opengl/LOGLCubemapsTab.h>
#include <liboscar-demos/learnopengl/advanced_opengl/LOGLFaceCullingTab.h>
#include <liboscar-demos/learnopengl/advanced_opengl/LOGLFramebuffersTab.h>
#include <liboscar-demos/learnopengl/getting_started/LOGLCoordinateSystemsTab.h>
#include <liboscar-demos/learnopengl/getting_started/LOGLHelloTriangleTab.h>
#include <liboscar-demos/learnopengl/getting_started/LOGLTexturingTab.h>
#include <liboscar-demos/learnopengl/guest/LOGLCSMTab.h>
#include <liboscar-demos/learnopengl/lighting/LOGLBasicLightingTab.h>
#include <liboscar-demos/learnopengl/lighting/LOGLLightingMapsTab.h>
#include <liboscar-demos/learnopengl/lighting/LOGLMultipleLightsTab.h>
#include <liboscar-demos/learnopengl/pbr/LOGLPBRDiffuseIrradianceTab.h>
#include <liboscar-demos/learnopengl/pbr/LOGLPBRLightingTab.h>
#include <liboscar-demos/learnopengl/pbr/LOGLPBRLightingTexturedTab.h>
#include <liboscar-demos/learnopengl/pbr/LOGLPBRSpecularIrradianceTab.h>
#include <liboscar-demos/learnopengl/pbr/LOGLPBRSpecularIrradianceTexturedTab.h>

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

#include <liboscar/utils/Typelist.h>

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

add_library(oscar_demos STATIC

    bookofshaders/BookOfShadersTab.cpp
    bookofshaders/BookOfShadersTab.h

    learnopengl/AdvancedLighting/LOGLBloomTab.cpp
    learnopengl/AdvancedLighting/LOGLBloomTab.h
    learnopengl/AdvancedLighting/LOGLDeferredShadingTab.cpp
    learnopengl/AdvancedLighting/LOGLDeferredShadingTab.h
    learnopengl/AdvancedLighting/LOGLGammaTab.cpp
    learnopengl/AdvancedLighting/LOGLGammaTab.h
    learnopengl/AdvancedLighting/LOGLHDRTab.cpp
    learnopengl/AdvancedLighting/LOGLHDRTab.h
    learnopengl/AdvancedLighting/LOGLNormalMappingTab.cpp
    learnopengl/AdvancedLighting/LOGLNormalMappingTab.h
    learnopengl/AdvancedLighting/LOGLParallaxMappingTab.cpp
    learnopengl/AdvancedLighting/LOGLParallaxMappingTab.h
    learnopengl/AdvancedLighting/LOGLPointShadowsTab.cpp
    learnopengl/AdvancedLighting/LOGLPointShadowsTab.h
    learnopengl/AdvancedLighting/LOGLShadowMappingTab.cpp
    learnopengl/AdvancedLighting/LOGLShadowMappingTab.h
    learnopengl/AdvancedLighting/LOGLSSAOTab.cpp
    learnopengl/AdvancedLighting/LOGLSSAOTab.h
    learnopengl/AdvancedOpenGL/LOGLBlendingTab.cpp
    learnopengl/AdvancedOpenGL/LOGLBlendingTab.h
    learnopengl/AdvancedOpenGL/LOGLCubemapsTab.cpp
    learnopengl/AdvancedOpenGL/LOGLCubemapsTab.h
    learnopengl/AdvancedOpenGL/LOGLFaceCullingTab.cpp
    learnopengl/AdvancedOpenGL/LOGLFaceCullingTab.h
    learnopengl/AdvancedOpenGL/LOGLFramebuffersTab.cpp
    learnopengl/AdvancedOpenGL/LOGLFramebuffersTab.h
    learnopengl/GettingStarted/LOGLCoordinateSystemsTab.cpp
    learnopengl/GettingStarted/LOGLCoordinateSystemsTab.h
    learnopengl/GettingStarted/LOGLHelloTriangleTab.cpp
    learnopengl/GettingStarted/LOGLHelloTriangleTab.h
    learnopengl/GettingStarted/LOGLTexturingTab.cpp
    learnopengl/GettingStarted/LOGLTexturingTab.h
    learnopengl/Guest/LOGLCSMTab.cpp
    learnopengl/Guest/LOGLCSMTab.h
    learnopengl/Lighting/LOGLBasicLightingTab.cpp
    learnopengl/Lighting/LOGLBasicLightingTab.h
    learnopengl/Lighting/LOGLLightingMapsTab.cpp
    learnopengl/Lighting/LOGLLightingMapsTab.h
    learnopengl/Lighting/LOGLMultipleLightsTab.cpp
    learnopengl/Lighting/LOGLMultipleLightsTab.h
    learnopengl/PBR/LOGLPBRDiffuseIrradianceTab.cpp
    learnopengl/PBR/LOGLPBRDiffuseIrradianceTab.h
    learnopengl/PBR/LOGLPBRLightingTab.cpp
    learnopengl/PBR/LOGLPBRLightingTab.h
    learnopengl/PBR/LOGLPBRLightingTexturedTab.cpp
    learnopengl/PBR/LOGLPBRLightingTexturedTab.h
    learnopengl/PBR/LOGLPBRSpecularIrradianceTab.cpp
    learnopengl/PBR/LOGLPBRSpecularIrradianceTab.h
    learnopengl/PBR/LOGLPBRSpecularIrradianceTexturedTab.cpp
    learnopengl/PBR/LOGLPBRSpecularIrradianceTexturedTab.h
    learnopengl/AdvancedLighting.h
    learnopengl/AdvancedOpenGL.h
    learnopengl/GettingStarted.h
    learnopengl/Guest.h
    learnopengl/Lighting.h
    learnopengl/PBR.h

    CookiecutterScreen.cpp
    CookiecutterScreen.h
    CustomWidgetsTab.cpp
    CustomWidgetsTab.h
    FrustrumCullingTab.cpp
    FrustrumCullingTab.h
    HittestTab.cpp
    HittestTab.h
    ImGuiDemoTab.cpp
    ImGuiDemoTab.h
    ImGuizmoDemoTab.cpp
    ImGuizmoDemoTab.h
    ImPlotDemoTab.cpp
    ImPlotDemoTab.h
    MandelbrotTab.cpp
    MandelbrotTab.h
    MeshGenTestTab.cpp
    MeshGenTestTab.h
    OscarDemosTabRegistry.cpp
    OscarDemosTabRegistry.h
    OscarDemoTabs.h
    SubMeshTab.cpp
    SubMeshTab.h
)
set_target_properties(oscar_demos PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(oscar_demos PUBLIC
    cxx_std_20
)
target_include_directories(oscar_demos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..      # so that `#include "oscar_demos/HEADER.h"` works
)
target_link_libraries(oscar_demos PUBLIC
    oscar
)

add_library(oscar_learnopengl STATIC

    AdvancedLighting/LOGLBloomTab.cpp
    AdvancedLighting/LOGLBloomTab.h
    AdvancedLighting/LOGLDeferredShadingTab.cpp
    AdvancedLighting/LOGLDeferredShadingTab.h
    AdvancedLighting/LOGLGammaTab.cpp
    AdvancedLighting/LOGLGammaTab.h
    AdvancedLighting/LOGLHDRTab.cpp
    AdvancedLighting/LOGLHDRTab.h
    AdvancedLighting/LOGLNormalMappingTab.cpp
    AdvancedLighting/LOGLNormalMappingTab.h
    AdvancedLighting/LOGLParallaxMappingTab.cpp
    AdvancedLighting/LOGLParallaxMappingTab.h
    AdvancedLighting/LOGLPointShadowsTab.cpp
    AdvancedLighting/LOGLPointShadowsTab.h
    AdvancedLighting/LOGLShadowMappingTab.cpp
    AdvancedLighting/LOGLShadowMappingTab.h
    AdvancedLighting/LOGLSSAOTab.cpp
    AdvancedLighting/LOGLSSAOTab.h

    AdvancedOpenGL/LOGLBlendingTab.cpp
    AdvancedOpenGL/LOGLBlendingTab.h
    AdvancedOpenGL/LOGLCubemapsTab.cpp
    AdvancedOpenGL/LOGLCubemapsTab.h
    AdvancedOpenGL/LOGLFaceCullingTab.cpp
    AdvancedOpenGL/LOGLFaceCullingTab.h
    AdvancedOpenGL/LOGLFramebuffersTab.cpp
    AdvancedOpenGL/LOGLFramebuffersTab.h

    GettingStarted/LOGLCoordinateSystemsTab.cpp
    GettingStarted/LOGLCoordinateSystemsTab.h
    GettingStarted/LOGLHelloTriangleTab.cpp
    GettingStarted/LOGLHelloTriangleTab.h
    GettingStarted/LOGLTexturingTab.cpp
    GettingStarted/LOGLTexturingTab.h

    Lighting/LOGLBasicLightingTab.cpp
    Lighting/LOGLBasicLightingTab.h
    Lighting/LOGLLightingMapsTab.cpp
    Lighting/LOGLLightingMapsTab.h
    Lighting/LOGLMultipleLightsTab.cpp
    Lighting/LOGLMultipleLightsTab.h

    PBR/LOGLPBRDiffuseIrradianceTab.cpp
    PBR/LOGLPBRDiffuseIrradianceTab.h
    PBR/LOGLPBRLightingTab.cpp
    PBR/LOGLPBRLightingTab.h
    PBR/LOGLPBRLightingTexturedTab.cpp
    PBR/LOGLPBRLightingTexturedTab.h
    PBR/LOGLPBRSpecularIrradianceTab.cpp
    PBR/LOGLPBRSpecularIrradianceTab.h
    PBR/LOGLPBRSpecularIrradianceTexturedTab.cpp
    PBR/LOGLPBRSpecularIrradianceTexturedTab.h

    AdvancedLighting.h
    AdvancedOpenGL.h
    GettingStarted.h
    LearnOpenGLHelpers.cpp
    LearnOpenGLHelpers.h
    LearnOpenGLTabRegistry.cpp
    LearnOpenGLTabRegistry.h
    LearnOpenGLTabs.h
    Lighting.h
    MouseCapturingCamera.h
    PBR.h
)

target_include_directories(oscar_learnopengl PUBLIC

    # so that `#include "oscar_learnopengl/HEADER.h"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_learnopengl PUBLIC
    oscar
)

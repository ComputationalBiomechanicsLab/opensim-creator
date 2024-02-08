add_library(oscar_learnopengl STATIC

    AdvancedLighting/LOGLBloomTab.cpp
    AdvancedLighting/LOGLBloomTab.hpp
    AdvancedLighting/LOGLDeferredShadingTab.cpp
    AdvancedLighting/LOGLDeferredShadingTab.hpp
    AdvancedLighting/LOGLGammaTab.cpp
    AdvancedLighting/LOGLGammaTab.hpp
    AdvancedLighting/LOGLHDRTab.cpp
    AdvancedLighting/LOGLHDRTab.hpp
    AdvancedLighting/LOGLNormalMappingTab.cpp
    AdvancedLighting/LOGLNormalMappingTab.hpp
    AdvancedLighting/LOGLParallaxMappingTab.cpp
    AdvancedLighting/LOGLParallaxMappingTab.hpp
    AdvancedLighting/LOGLPointShadowsTab.cpp
    AdvancedLighting/LOGLPointShadowsTab.hpp
    AdvancedLighting/LOGLShadowMappingTab.cpp
    AdvancedLighting/LOGLShadowMappingTab.hpp
    AdvancedLighting/LOGLSSAOTab.cpp
    AdvancedLighting/LOGLSSAOTab.hpp

    AdvancedOpenGL/LOGLBlendingTab.cpp
    AdvancedOpenGL/LOGLBlendingTab.hpp
    AdvancedOpenGL/LOGLCubemapsTab.cpp
    AdvancedOpenGL/LOGLCubemapsTab.hpp
    AdvancedOpenGL/LOGLFaceCullingTab.cpp
    AdvancedOpenGL/LOGLFaceCullingTab.hpp
    AdvancedOpenGL/LOGLFramebuffersTab.cpp
    AdvancedOpenGL/LOGLFramebuffersTab.hpp

    GettingStarted/LOGLCoordinateSystemsTab.cpp
    GettingStarted/LOGLCoordinateSystemsTab.hpp
    GettingStarted/LOGLHelloTriangleTab.cpp
    GettingStarted/LOGLHelloTriangleTab.hpp
    GettingStarted/LOGLTexturingTab.cpp
    GettingStarted/LOGLTexturingTab.hpp

    Lighting/LOGLBasicLightingTab.cpp
    Lighting/LOGLBasicLightingTab.hpp
    Lighting/LOGLLightingMapsTab.cpp
    Lighting/LOGLLightingMapsTab.hpp
    Lighting/LOGLMultipleLightsTab.cpp
    Lighting/LOGLMultipleLightsTab.hpp

    PBR/LOGLPBRDiffuseIrradianceTab.cpp
    PBR/LOGLPBRDiffuseIrradianceTab.hpp
    PBR/LOGLPBRLightingTab.cpp
    PBR/LOGLPBRLightingTab.hpp
    PBR/LOGLPBRLightingTexturedTab.cpp
    PBR/LOGLPBRLightingTexturedTab.hpp
    PBR/LOGLPBRSpecularIrradianceTab.cpp
    PBR/LOGLPBRSpecularIrradianceTab.hpp
    PBR/LOGLPBRSpecularIrradianceTexturedTab.cpp
    PBR/LOGLPBRSpecularIrradianceTexturedTab.hpp

    LearnOpenGLHelpers.cpp
    LearnOpenGLHelpers.hpp
    LearnOpenGLTabRegistry.cpp
    LearnOpenGLTabRegistry.hpp
    LearnOpenGLTabs.hpp
    MouseCapturingCamera.hpp
)

target_include_directories(oscar_learnopengl PUBLIC

    # so that `#include "oscar_learnopengl/HEADER.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_learnopengl PUBLIC
    oscar
)

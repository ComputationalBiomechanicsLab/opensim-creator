add_library(oscar_learnopengl STATIC

    Tabs/AdvancedLighting/LOGLBloomTab.cpp
    Tabs/AdvancedLighting/LOGLBloomTab.hpp
    Tabs/AdvancedLighting/LOGLDeferredShadingTab.cpp
    Tabs/AdvancedLighting/LOGLDeferredShadingTab.hpp
    Tabs/AdvancedLighting/LOGLGammaTab.cpp
    Tabs/AdvancedLighting/LOGLGammaTab.hpp
    Tabs/AdvancedLighting/LOGLHDRTab.cpp
    Tabs/AdvancedLighting/LOGLHDRTab.hpp
    Tabs/AdvancedLighting/LOGLNormalMappingTab.cpp
    Tabs/AdvancedLighting/LOGLNormalMappingTab.hpp
    Tabs/AdvancedLighting/LOGLParallaxMappingTab.cpp
    Tabs/AdvancedLighting/LOGLParallaxMappingTab.hpp
    Tabs/AdvancedLighting/LOGLPointShadowsTab.cpp
    Tabs/AdvancedLighting/LOGLPointShadowsTab.hpp
    Tabs/AdvancedLighting/LOGLShadowMappingTab.cpp
    Tabs/AdvancedLighting/LOGLShadowMappingTab.hpp
    Tabs/AdvancedLighting/LOGLSSAOTab.cpp
    Tabs/AdvancedLighting/LOGLSSAOTab.hpp

    Tabs/AdvancedOpenGL/LOGLBlendingTab.cpp
    Tabs/AdvancedOpenGL/LOGLBlendingTab.hpp
    Tabs/AdvancedOpenGL/LOGLCubemapsTab.cpp
    Tabs/AdvancedOpenGL/LOGLCubemapsTab.hpp
    Tabs/AdvancedOpenGL/LOGLFaceCullingTab.cpp
    Tabs/AdvancedOpenGL/LOGLFaceCullingTab.hpp
    Tabs/AdvancedOpenGL/LOGLFramebuffersTab.cpp
    Tabs/AdvancedOpenGL/LOGLFramebuffersTab.hpp

    Tabs/GettingStarted/LOGLCoordinateSystemsTab.cpp
    Tabs/GettingStarted/LOGLCoordinateSystemsTab.hpp
    Tabs/GettingStarted/LOGLHelloTriangleTab.cpp
    Tabs/GettingStarted/LOGLHelloTriangleTab.hpp
    Tabs/GettingStarted/LOGLTexturingTab.cpp
    Tabs/GettingStarted/LOGLTexturingTab.hpp

    Tabs/Lighting/LOGLBasicLightingTab.cpp
    Tabs/Lighting/LOGLBasicLightingTab.hpp
    Tabs/Lighting/LOGLLightingMapsTab.cpp
    Tabs/Lighting/LOGLLightingMapsTab.hpp
    Tabs/Lighting/LOGLMultipleLightsTab.cpp
    Tabs/Lighting/LOGLMultipleLightsTab.hpp

    Tabs/PBR/LOGLPBRDiffuseIrradianceTab.cpp
    Tabs/PBR/LOGLPBRDiffuseIrradianceTab.hpp
    Tabs/PBR/LOGLPBRLightingTab.cpp
    Tabs/PBR/LOGLPBRLightingTab.hpp
    Tabs/PBR/LOGLPBRLightingTexturedTab.cpp
    Tabs/PBR/LOGLPBRLightingTexturedTab.hpp
    Tabs/PBR/LOGLPBRSpecularIrradianceTab.cpp
    Tabs/PBR/LOGLPBRSpecularIrradianceTab.hpp
    Tabs/PBR/LOGLPBRSpecularIrradianceTexturedTab.cpp
    Tabs/PBR/LOGLPBRSpecularIrradianceTexturedTab.hpp

    LearnOpenGLTabRegistry.cpp
 "LearnOpenGLHelpers.cpp")

target_compile_options(oscar_learnopengl PRIVATE

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
)

target_include_directories(oscar_learnopengl PUBLIC

    # so that `#include "oscar_learnopengl/HEADER.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_learnopengl PUBLIC

    oscar
)

#pragma once

#include <liboscar-demos/bookofshaders/book_of_shaders_tab.h>

#include <liboscar-demos/learnopengl/advanced_lighting/logl_bloom_tab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/logl_deferred_shading_tab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/logl_gamma_tab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/logl_hdr_tab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/logl_normal_mapping_tab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/logl_parallax_mapping_tab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/logl_point_shadows_tab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/logl_shadow_mapping_tab.h>
#include <liboscar-demos/learnopengl/advanced_lighting/logl_ssao_tab.h>
#include <liboscar-demos/learnopengl/advanced_opengl/logl_blending_tab.h>
#include <liboscar-demos/learnopengl/advanced_opengl/logl_cubemaps_tab.h>
#include <liboscar-demos/learnopengl/advanced_opengl/logl_face_culling_tab.h>
#include <liboscar-demos/learnopengl/advanced_opengl/logl_framebuffers_tab.h>
#include <liboscar-demos/learnopengl/getting_started/logl_coordinate_systems_tab.h>
#include <liboscar-demos/learnopengl/getting_started/logl_hello_triangle_tab.h>
#include <liboscar-demos/learnopengl/getting_started/logl_texturing_tab.h>
#include <liboscar-demos/learnopengl/guest/logl_csm_tab.h>
#include <liboscar-demos/learnopengl/lighting/logl_basic_lighting_tab.h>
#include <liboscar-demos/learnopengl/lighting/logl_lighting_maps_tab.h>
#include <liboscar-demos/learnopengl/lighting/logl_multiple_lights_tab.h>
#include <liboscar-demos/learnopengl/pbr/logl_pbr_diffuse_irradiance_tab.h>
#include <liboscar-demos/learnopengl/pbr/logl_pbr_lighting_tab.h>
#include <liboscar-demos/learnopengl/pbr/logl_pbr_lighting_textured_tab.h>
#include <liboscar-demos/learnopengl/pbr/logl_pbr_specular_irradiance_tab.h>
#include <liboscar-demos/learnopengl/pbr/logl_pbr_specular_irradiance_textured_tab.h>

#include <liboscar-demos/custom_widgets_tab.h>
#include <liboscar-demos/drawing_test_tab.h>
#include <liboscar-demos/frustum_culling_tab.h>
#include <liboscar-demos/hittest_tab.h>
#include <liboscar-demos/im_gui_demo_tab.h>
#include <liboscar-demos/im_guizmo_demo_tab.h>
#include <liboscar-demos/im_plot_demo_tab.h>
#include <liboscar-demos/mandelbrot_tab.h>
#include <liboscar-demos/mesh_gen_test_tab.h>
#include <liboscar-demos/sub_mesh_demo.h>

#include <liboscar/utilities/typelist.h>

namespace osc
{
    using OscarDemosTypelist = Typelist<
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
        SubMeshDemo
    >;
}

#pragma once

#include <libopensimcreator/ui/mesh_hittest_tab.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_tab.h>
#include <libopensimcreator/ui/mesh_warper/mesh_warping_tab.h>
#include <libopensimcreator/ui/model_editor/model_editor_tab.h>
#include <libopensimcreator/ui/model_warper/model_warper_tab.h>
#include <libopensimcreator/ui/preview_experimental_data/preview_experimental_data_tab.h>
#include <libopensimcreator/ui/renderer_geometry_shader_tab.h>
#include <libopensimcreator/ui/renderer_perf_testing_tab.h>
#include <libopensimcreator/ui/tps_2d_tab.h>

#include <liboscar/utilities/typelist.h>

namespace osc
{
    using OpenSimCreatorTabs = Typelist<
        PreviewExperimentalDataTab,

        MeshWarpingTab,
        MeshImporterTab,
        ModelEditorTab,
        ModelWarperTab,
        MeshHittestTab,
        RendererGeometryShaderTab,
        RendererPerfTestingTab,
        TPS2DTab
    >;
}

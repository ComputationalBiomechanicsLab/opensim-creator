#pragma once

#include <libopensimcreator/UI/FrameDefinition/FrameDefinitionTab.h>
#include <libopensimcreator/UI/MeshHittestTab.h>
#include <libopensimcreator/UI/MeshImporter/MeshImporterTab.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTab.h>
#include <libopensimcreator/UI/ModelEditor/ModelEditorTab.h>
#include <libopensimcreator/UI/ModelWarper/ModelWarperTab.h>
#include <libopensimcreator/UI/ModelWarperV3/ModelWarperV3Tab.h>
#include <libopensimcreator/UI/PreviewExperimentalData/PreviewExperimentalDataTab.h>
#include <libopensimcreator/UI/RendererGeometryShaderTab.h>
#include <libopensimcreator/UI/RendererPerfTestingTab.h>
#include <libopensimcreator/UI/TPS2DTab.h>

#include <liboscar/Utils/Typelist.h>

namespace osc
{
    using OpenSimCreatorTabs = Typelist<
        PreviewExperimentalDataTab,

        MeshWarpingTab,
        mi::MeshImporterTab,
        ModelEditorTab,
        mow::ModelWarperTab,
        ModelWarperV3Tab,
        FrameDefinitionTab,
        MeshHittestTab,
        RendererGeometryShaderTab,
        RendererPerfTestingTab,
        TPS2DTab
    >;
}

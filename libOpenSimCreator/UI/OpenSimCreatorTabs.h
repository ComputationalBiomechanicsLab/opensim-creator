#pragma once

#include <libOpenSimCreator/UI/FrameDefinition/FrameDefinitionTab.h>
#include <libOpenSimCreator/UI/MeshHittestTab.h>
#include <libOpenSimCreator/UI/MeshImporter/MeshImporterTab.h>
#include <libOpenSimCreator/UI/MeshWarper/MeshWarpingTab.h>
#include <libOpenSimCreator/UI/ModelEditor/ModelEditorTab.h>
#include <libOpenSimCreator/UI/ModelWarper/ModelWarperTab.h>
#include <libOpenSimCreator/UI/ModelWarperV3/ModelWarperV3Tab.h>
#include <libOpenSimCreator/UI/PreviewExperimentalData/PreviewExperimentalDataTab.h>
#include <libOpenSimCreator/UI/RendererGeometryShaderTab.h>
#include <libOpenSimCreator/UI/RendererPerfTestingTab.h>
#include <libOpenSimCreator/UI/TPS2DTab.h>

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

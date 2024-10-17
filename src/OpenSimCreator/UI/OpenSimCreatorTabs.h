#pragma once

#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionTab.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterTab.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTab.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.h>
#include <OpenSimCreator/UI/ModelWarper/ModelWarperTab.h>
#include <OpenSimCreator/UI/PreviewExperimentalData/PreviewExperimentalDataTab.h>
#include <OpenSimCreator/UI/MeshHittestTab.h>
#include <OpenSimCreator/UI/RendererGeometryShaderTab.h>
#include <OpenSimCreator/UI/TPS2DTab.h>

#include <oscar/Utils/Typelist.h>

namespace osc
{
    using OpenSimCreatorTabs = Typelist<
        PreviewExperimentalDataTab,

        MeshWarpingTab,
        mi::MeshImporterTab,
        ModelEditorTab,
        mow::ModelWarperTab,
        FrameDefinitionTab,
        MeshHittestTab,
        RendererGeometryShaderTab,
        TPS2DTab
    >;
}

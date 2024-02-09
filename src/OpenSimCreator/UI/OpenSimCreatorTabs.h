#pragma once

#include <OpenSimCreator/UI/Experimental/MeshHittestTab.h>
#include <OpenSimCreator/UI/Experimental/RendererGeometryShaderTab.h>
#include <OpenSimCreator/UI/Experimental/TPS2DTab.h>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionTab.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTab.h>
#include <OpenSimCreator/UI/ModelWarper/ModelWarperTab.h>

#include <oscar/Utils/Typelist.h>

namespace osc
{
    using OpenSimCreatorTabs = Typelist<
        MeshHittestTab,
        RendererGeometryShaderTab,
        TPS2DTab,
        MeshWarpingTab,
        mow::ModelWarperTab,
        FrameDefinitionTab
    >;
}

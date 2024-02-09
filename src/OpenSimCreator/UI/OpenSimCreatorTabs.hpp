#pragma once

#include <OpenSimCreator/UI/Experimental/MeshHittestTab.hpp>
#include <OpenSimCreator/UI/Experimental/RendererGeometryShaderTab.hpp>
#include <OpenSimCreator/UI/Experimental/TPS2DTab.hpp>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionTab.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTab.hpp>
#include <OpenSimCreator/UI/ModelWarper/ModelWarperTab.hpp>

#include <oscar/Utils/Typelist.hpp>

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

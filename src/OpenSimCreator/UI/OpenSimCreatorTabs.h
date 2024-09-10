#pragma once

#include <OpenSimCreator/UI/Experimental/PreviewExperimentalDataTab.h>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionTab.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTab.h>
#include <OpenSimCreator/UI/ModelWarper/ModelWarperTab.h>

#include <oscar/Utils/Typelist.h>

namespace osc
{
    using OpenSimCreatorTabs = Typelist<
        PreviewExperimentalDataTab,

        MeshWarpingTab,
        mow::ModelWarperTab,
        FrameDefinitionTab
    >;
}

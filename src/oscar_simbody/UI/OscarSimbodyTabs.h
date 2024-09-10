#pragma once

#include <oscar_simbody/UI/MeshHittestTab.h>
#include <oscar_simbody/UI/RendererGeometryShaderTab.h>
#include <oscar_simbody/UI/TPS2DTab.h>

#include <oscar/Utils/Typelist.h>

namespace osc
{
    using OscarSimbodyTabs = Typelist<
        MeshHittestTab,
        RendererGeometryShaderTab,
        TPS2DTab
    >;
}

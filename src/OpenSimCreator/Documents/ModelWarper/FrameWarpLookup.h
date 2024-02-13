#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarp.h>

#include <concepts>
#include <filesystem>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    class FrameWarpLookup final {
    public:
        FrameWarpLookup() = default;
        FrameWarpLookup(
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&,
            ModelWarpConfiguration const&
        );

        template<std::derived_from<IFrameWarp> TMeshWarp = IFrameWarp>
        TMeshWarp const* find(std::string const& meshComponentAbsPath) const
        {
            return dynamic_cast<TMeshWarp const*>(find<IFrameWarp>(meshComponentAbsPath));
        }

        template<>
        IFrameWarp const* find<IFrameWarp>(std::string const&) const
        {
            return nullptr;  // TODO
        }
    };
}

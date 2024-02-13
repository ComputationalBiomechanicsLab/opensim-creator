#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarp.h>

#include <oscar/Utils/ClonePtr.h>

#include <concepts>
#include <filesystem>
#include <unordered_map>

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
        IFrameWarp const* find<IFrameWarp>(std::string const& absPath) const
        {
            if (auto const it = m_AbsPathToWarpLUT.find(absPath); it != m_AbsPathToWarpLUT.end()) {
                return it->second.get();
            }
            else {
                return nullptr;
            }
        }

    private:
        std::unordered_map<std::string, ClonePtr<IFrameWarp>> m_AbsPathToWarpLUT;
    };
}

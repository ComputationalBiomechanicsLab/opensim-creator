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

        template<std::derived_from<IFrameWarp> FrameWarp = IFrameWarp>
        FrameWarp const* find(std::string const& absPath) const
        {
            return dynamic_cast<FrameWarp const*>(lookup(absPath));
        }

    private:
        IFrameWarp const* lookup(std::string const& absPath) const
        {
            if (auto const it = m_AbsPathToWarpLUT.find(absPath); it != m_AbsPathToWarpLUT.end()) {
                return it->second.get();
            }
            else {
                return nullptr;
            }
        }

        std::unordered_map<std::string, ClonePtr<IFrameWarp>> m_AbsPathToWarpLUT;
    };
}

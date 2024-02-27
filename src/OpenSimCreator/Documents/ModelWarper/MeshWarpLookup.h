#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IMeshWarp.h>

#include <oscar/Utils/ClonePtr.h>

#include <concepts>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    class MeshWarpLookup final {
    public:
        MeshWarpLookup() = default;
        MeshWarpLookup(
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&,
            ModelWarpConfiguration const&
        );

        template<std::derived_from<IMeshWarp> TMeshWarp = IMeshWarp>
        TMeshWarp const* find(std::string const& meshComponentAbsPath) const
        {
            return dynamic_cast<TMeshWarp const*>(lookup(meshComponentAbsPath));
        }

        friend bool operator==(MeshWarpLookup const&, MeshWarpLookup const&) = default;
    private:
        IMeshWarp const* lookup(std::string const& absPath) const
        {
            if (auto const it = m_AbsPathToWarpLUT.find(absPath); it != m_AbsPathToWarpLUT.end()) {
                return it->second.get();
            }
            else {
                return nullptr;
            }
        }

        std::unordered_map<std::string, ClonePtr<IMeshWarp>> m_AbsPathToWarpLUT;
    };
}

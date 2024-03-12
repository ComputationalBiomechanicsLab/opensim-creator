#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IPointWarperFactory.h>

#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/ClonePtr.h>

#include <concepts>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    // runtime `ComponentAbsPath --> IPointWarperFactory` lookup that the warping
    // engine (and UI) use to find (and validate) `IPointWarperFactory`s that are
    // associated to components in an OpenSim model
    class PointWarperFactories final {
    public:
        // constructs an empty lookup
        PointWarperFactories() = default;

        // constructs a lookup that, given the inputs, is as populated as possible (i.e.
        // actually tries to figure out which concrete point warpers to use, etc.)
        PointWarperFactories(
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&,
            ModelWarpConfiguration const&
        );

        template<std::derived_from<IPointWarperFactory> TMeshWarp = IPointWarperFactory>
        TMeshWarp const* find(std::string const& meshComponentAbsPath) const
        {
            return dynamic_cast<TMeshWarp const*>(lookup(meshComponentAbsPath));
        }

    private:
        IPointWarperFactory const* lookup(std::string const& absPath) const
        {
            if (auto const* ptr = try_find(m_AbsPathToWarpLUT, absPath)) {
                return ptr->get();
            }
            else {
                return nullptr;
            }
        }

        std::unordered_map<std::string, ClonePtr<IPointWarperFactory>> m_AbsPathToWarpLUT;
    };
}

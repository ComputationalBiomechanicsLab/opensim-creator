#pragma once

#include <libopensimcreator/Documents/ModelWarper/IPointWarperFactory.h>

#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/ClonePtr.h>

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
            const std::filesystem::path& osimFileLocation,
            const OpenSim::Model&,
            const ModelWarpConfiguration&
        );

        template<std::derived_from<IPointWarperFactory> TMeshWarp = IPointWarperFactory>
        const TMeshWarp* find(const std::string& meshComponentAbsPath) const
        {
            return dynamic_cast<const TMeshWarp*>(lookup(meshComponentAbsPath));
        }

    private:
        const IPointWarperFactory* lookup(const std::string& absPath) const
        {
            if (const auto* ptr = lookup_or_nullptr(m_AbsPathToWarpLUT, absPath)) {
                return ptr->get();
            }
            else {
                return nullptr;
            }
        }

        std::unordered_map<std::string, ClonePtr<IPointWarperFactory>> m_AbsPathToWarpLUT;
    };
}

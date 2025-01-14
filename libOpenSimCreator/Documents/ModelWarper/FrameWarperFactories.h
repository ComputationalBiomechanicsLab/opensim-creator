#pragma once

#include <libOpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>

#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/ClonePtr.h>

#include <concepts>
#include <filesystem>
#include <unordered_map>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    // runtime `ComponentAbsPath --> IFrameWarperFactory` lookup that the warping
    // engine (and UI) use to find (and validate) `IFrameWarperFactory`s that are
    // associated to components in an OpenSim model
    class FrameWarperFactories final {
    public:
        // constructs an empty lookup
        FrameWarperFactories() = default;

        // constructs a lookup that, given the inputs, is as populated as possible (i.e.
        // actually tries to figure out which concrete frame warpers to use, etc.)
        FrameWarperFactories(
            const std::filesystem::path& osimFileLocation,
            const OpenSim::Model&,
            const ModelWarpConfiguration&
        );

        template<std::derived_from<IFrameWarperFactory> FrameWarp = IFrameWarperFactory>
        const FrameWarp* find(const std::string& absPath) const
        {
            return dynamic_cast<const FrameWarp*>(lookup(absPath));
        }

        [[nodiscard]] bool empty() const { return m_AbsPathToWarpLUT.empty(); }

    private:
        const IFrameWarperFactory* lookup(const std::string& absPath) const
        {
            if (const auto* ptr = lookup_or_nullptr(m_AbsPathToWarpLUT, absPath)) {
                return ptr->get();
            }
            else {
                return nullptr;
            }
        }

        std::unordered_map<std::string, ClonePtr<IFrameWarperFactory>> m_AbsPathToWarpLUT;
    };
}

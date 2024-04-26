#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>

#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/ClonePtr.h>

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
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&,
            ModelWarpConfiguration const&
        );

        template<std::derived_from<IFrameWarperFactory> FrameWarp = IFrameWarperFactory>
        FrameWarp const* find(std::string const& absPath) const
        {
            return dynamic_cast<FrameWarp const*>(lookup(absPath));
        }

        [[nodiscard]] bool empty() const { return m_AbsPathToWarpLUT.empty(); }

    private:
        IFrameWarperFactory const* lookup(std::string const& absPath) const
        {
            if (auto const* ptr = find_or_nullptr(m_AbsPathToWarpLUT, absPath)) {
                return ptr->get();
            }
            else {
                return nullptr;
            }
        }

        std::unordered_map<std::string, ClonePtr<IFrameWarperFactory>> m_AbsPathToWarpLUT;
    };
}

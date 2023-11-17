#pragma once

#include <oscar/Utils/UID.hpp>

namespace osc { class SceneEl; }

namespace osc
{
    // virtual interface to something that can be used to lookup scene elements in
    // some larger document
    class ISceneElLookup {
    protected:
        ISceneElLookup() = default;
        ISceneElLookup(ISceneElLookup const&) = default;
        ISceneElLookup(ISceneElLookup&&) noexcept = default;
        ISceneElLookup& operator=(ISceneElLookup const&) = default;
        ISceneElLookup& operator=(ISceneElLookup&&) noexcept = default;
    public:
        virtual ~ISceneElLookup() noexcept = default;

        SceneEl const* find(UID id) const
        {
            return implFind(id);
        }
    private:
        virtual SceneEl const* implFind(UID) const = 0;
    };
}

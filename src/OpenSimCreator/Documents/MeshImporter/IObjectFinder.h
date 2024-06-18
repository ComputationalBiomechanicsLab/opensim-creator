#pragma once

#include <oscar/Utils/UID.h>

namespace osc::mi { class MIObject; }

namespace osc::mi
{
    // virtual interface to something that can be used to lookup objects in some larger document
    class IObjectFinder {
    protected:
        IObjectFinder() = default;
        IObjectFinder(const IObjectFinder&) = default;
        IObjectFinder(IObjectFinder&&) noexcept = default;
        IObjectFinder& operator=(const IObjectFinder&) = default;
        IObjectFinder& operator=(IObjectFinder&&) noexcept = default;
    public:
        virtual ~IObjectFinder() noexcept = default;

        MIObject const* find(UID id) const
        {
            return implFind(id);
        }
    private:
        virtual MIObject const* implFind(UID) const = 0;
    };
}

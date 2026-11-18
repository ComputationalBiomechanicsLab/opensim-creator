#pragma once

#include <liboscar/utilities/uid.h>

namespace osc
{
    class MiObject;

    // virtual interface to something that can be used to lookup objects in some larger document
    class MiObjectFinder {
    protected:
        MiObjectFinder() = default;
        MiObjectFinder(const MiObjectFinder&) = default;
        MiObjectFinder(MiObjectFinder&&) noexcept = default;
        MiObjectFinder& operator=(const MiObjectFinder&) = default;
        MiObjectFinder& operator=(MiObjectFinder&&) noexcept = default;

    public:
        virtual ~MiObjectFinder() noexcept = default;

        const MiObject* find(UID id) const { return impl_find(id); }

    private:
        virtual const MiObject* impl_find(UID) const = 0;
    };
}

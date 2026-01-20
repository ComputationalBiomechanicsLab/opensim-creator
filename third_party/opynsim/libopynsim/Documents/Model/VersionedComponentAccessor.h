#pragma once

#include <libopynsim/Documents/Model/ComponentAccessor.h>
#include <liboscar/utils/uid.h>

namespace osc
{
    class VersionedComponentAccessor : public ComponentAccessor {
    public:
        UID getComponentVersion() const { return implGetComponentVersion(); }
        void setComponentVersion(UID id) { implSetComponentVersion(id); }

    private:
        // Implementors may return a `UID` that uniquely identifies the current version of the component.
        virtual UID implGetComponentVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return UID{};
        }

        // Implementors may use this to manually set the version of the component (sometimes useful for caching)
        virtual void implSetComponentVersion(UID) {}
    };
}

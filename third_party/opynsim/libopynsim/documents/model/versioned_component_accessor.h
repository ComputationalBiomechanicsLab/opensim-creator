#pragma once

#include <libopynsim/documents/model/component_accessor.h>
#include <liboscar/utilities/uid.h>

namespace opyn
{
    class VersionedComponentAccessor : public ComponentAccessor {
    public:
        osc::UID getComponentVersion() const { return implGetComponentVersion(); }
        void setComponentVersion(osc::UID id) { implSetComponentVersion(id); }

    private:
        // Implementors may return a `UID` that uniquely identifies the current version of the component.
        virtual osc::UID implGetComponentVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return osc::UID{};
        }

        // Implementors may use this to manually set the version of the component (sometimes useful for caching)
        virtual void implSetComponentVersion(osc::UID) {}
    };
}

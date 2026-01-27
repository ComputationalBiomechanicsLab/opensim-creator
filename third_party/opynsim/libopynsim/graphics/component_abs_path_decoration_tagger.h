#pragma once

#include <liboscar/utilities/string_name.h>

namespace OpenSim { class Component; }
namespace osc { struct SceneDecoration; }

namespace opyn
{
    // functor class that sets a decoration's ID the component's abs path
    class ComponentAbsPathDecorationTagger final {
    public:
        void operator()(const OpenSim::Component&, osc::SceneDecoration&);
    private:
        const OpenSim::Component* m_LastComponent = nullptr;
        osc::StringName m_ID;
    };
}

#pragma once

#include <oscar/Utils/StringName.h>

namespace OpenSim { class Component; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    // functor class that sets a decoration's ID the the component's abs path
    class ComponentAbsPathDecorationTagger final {
    public:
        void operator()(const OpenSim::Component&, SceneDecoration&);
    private:
        const OpenSim::Component* m_LastComponent = nullptr;
        StringName m_ID;
    };
}

#pragma once

#include <string>

namespace OpenSim { class Component; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    // functor class that sets a decoration's ID the the component's abs path
    class ComponentAbsPathDecorationTagger final {
    public:
        void operator()(const OpenSim::Component&, SceneDecoration&);
    private:
        OpenSim::Component const* m_LastComponent = nullptr;
        std::string m_ID;
    };
}

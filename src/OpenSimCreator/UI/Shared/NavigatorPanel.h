#pragma once

#include <oscar/UI/Panels/Panel.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class IModelStatePair; }

namespace osc
{
    class NavigatorPanel final : public Panel {
    public:
        explicit NavigatorPanel(
            std::string_view panelName,
            std::shared_ptr<IModelStatePair>,
            std::function<void(const OpenSim::ComponentPath&)> onRightClick = [](const auto&){}
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

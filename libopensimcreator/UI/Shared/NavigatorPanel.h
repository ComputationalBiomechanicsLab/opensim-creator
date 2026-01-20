#pragma once

#include <liboscar/ui/panels/panel.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class ModelStatePair; }

namespace osc
{
    class NavigatorPanel final : public Panel {
    public:
        explicit NavigatorPanel(
            Widget* parent,
            std::string_view panelName,
            std::shared_ptr<ModelStatePair>,
            std::function<void(const OpenSim::ComponentPath&)> onRightClick = [](const auto&){}
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

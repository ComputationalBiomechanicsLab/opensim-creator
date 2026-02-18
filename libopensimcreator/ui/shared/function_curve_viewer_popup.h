#pragma once

#include <liboscar/ui/panels/panel.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Function; }
namespace opyn { class VersionedComponentAccessor; }

namespace osc
{

    class FunctionCurveViewerPanel final : public Panel {
    public:
        explicit FunctionCurveViewerPanel(
            Widget* parent,
            std::string_view panelName,
            std::shared_ptr<const opyn::VersionedComponentAccessor> targetComponent,
            std::function<const OpenSim::Function*()> functionGetter
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

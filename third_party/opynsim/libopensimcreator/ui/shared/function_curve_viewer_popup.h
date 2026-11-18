#pragma once

#include <liboscar/ui/panels/panel.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim
{
    class Function;
}

namespace opyn
{
    class VersionedComponentAccessor;
}

namespace osc
{

    class FunctionCurveViewerPanel final : public Panel {
    public:
        explicit FunctionCurveViewerPanel(
            Widget* parent,
            std::string_view panel_name,
            std::shared_ptr<const opyn::VersionedComponentAccessor> target_component,
            std::function<const OpenSim::Function*()> function_getter);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

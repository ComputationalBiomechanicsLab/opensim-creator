#pragma once

#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <string_view>

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class Widget;

    class CoordinateEditorPanel final : public Panel {
    public:
        explicit CoordinateEditorPanel(
            Widget* parent, std::string_view panel_name, std::shared_ptr<opyn::ModelStatePair>);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

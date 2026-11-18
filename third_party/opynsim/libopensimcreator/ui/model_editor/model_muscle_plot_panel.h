#pragma once

#include <liboscar/platform/widget.h>
#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <string_view>

namespace OpenSim
{
    class ComponentPath;
}
namespace osc
{
    class UndoableModelStatePair;
}

namespace osc
{
    class ModelMusclePlotPanel final : public Panel {
    public:
        explicit ModelMusclePlotPanel(
            Widget*, std::shared_ptr<UndoableModelStatePair>, std::string_view panel_name);
        explicit ModelMusclePlotPanel(
            Widget*,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view panel_name,
            const OpenSim::ComponentPath& coord_path,
            const OpenSim::ComponentPath& muscle_path);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

#pragma once

#include <oscar/Platform/Widget.h>
#include <oscar/UI/Panels/Panel.h>

#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelMusclePlotPanel final : public Panel {
    public:
        explicit ModelMusclePlotPanel(
            Widget&,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view panelName
        );
        explicit ModelMusclePlotPanel(
            Widget&,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view panelName,
            const OpenSim::ComponentPath& coordPath,
            const OpenSim::ComponentPath& musclePath
        );
    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

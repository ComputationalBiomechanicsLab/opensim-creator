#pragma once

#include <oscar/UI/Panels/Panel.h>

#include <memory>
#include <string_view>

namespace osc { class IModelStatePair; }

namespace osc
{
    class OutputWatchesPanel final : public Panel {
    public:
        explicit OutputWatchesPanel(
            std::string_view panelName,
            std::shared_ptr<const IModelStatePair>
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

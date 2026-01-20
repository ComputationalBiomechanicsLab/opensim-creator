#pragma once

#include <liboscar/ui/popups/popup.h>

#include <memory>
#include <string_view>

namespace osc { class ModelStatePair; }

namespace osc
{
    class ExportPointsPopup final : public Popup {
    public:
        explicit ExportPointsPopup(
            Widget* parent,
            std::string_view popupName,
            std::shared_ptr<const ModelStatePair>
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

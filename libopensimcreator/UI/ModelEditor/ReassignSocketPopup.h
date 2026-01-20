#pragma once

#include <liboscar/ui/popups/popup.h>

#include <memory>
#include <string_view>

namespace osc { class ModelStatePair; }

namespace osc
{
    class ReassignSocketPopup final : public Popup {
    public:
        explicit ReassignSocketPopup(
            Widget* parent,
            std::string_view popupName,
            std::shared_ptr<ModelStatePair>,
            std::string_view componentAbsPath,
            std::string_view socketName
        );

    private:
        void impl_draw_content() final;
        void impl_on_close() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

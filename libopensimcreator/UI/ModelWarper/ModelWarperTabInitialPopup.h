#pragma once

#include <liboscar/UI/Popups/Popup.h>

#include <memory>
#include <string_view>

namespace osc
{
    class ModelWarperTabInitialPopup final : public Popup {
    public:
        explicit ModelWarperTabInitialPopup(
            Widget* parent,
            std::string_view popupName
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

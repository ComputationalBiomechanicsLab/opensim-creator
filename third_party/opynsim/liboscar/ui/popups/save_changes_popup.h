#pragma once

#include <liboscar/ui/popups/popup.h>

namespace osc
{
    class Widget;
    struct SaveChangesPopupConfig;

    class SaveChangesPopup final : public Popup {
    public:
        explicit SaveChangesPopup(Widget* parent, const SaveChangesPopupConfig&);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

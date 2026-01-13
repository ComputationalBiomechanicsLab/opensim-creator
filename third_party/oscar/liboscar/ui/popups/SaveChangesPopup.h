#pragma once

#include <liboscar/ui/popups/Popup.h>

namespace osc { struct SaveChangesPopupConfig; }
namespace osc { class Widget; }

namespace osc
{
    class SaveChangesPopup final : public Popup {
    public:
        explicit SaveChangesPopup(
            Widget* parent,
            const SaveChangesPopupConfig&
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

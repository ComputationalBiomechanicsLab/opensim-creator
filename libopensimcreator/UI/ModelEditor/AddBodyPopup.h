#pragma once

#include <liboscar/UI/Popups/Popup.h>

#include <memory>
#include <string_view>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class AddBodyPopup final : public Popup {
    public:
        explicit AddBodyPopup(
            Widget* parent,
            std::string_view popupName,
            std::shared_ptr<IModelStatePair>
        );

    private:
        void impl_draw_content() final;
        void impl_on_close() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

#pragma once

#include <liboscar/ui/popups/popup.h>

#include <memory>
#include <string_view>

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class Widget;

    class AddBodyPopup final : public Popup {
    public:
        explicit AddBodyPopup(
            Widget* parent, std::string_view popup_name, std::shared_ptr<opyn::ModelStatePair>);

    private:
        void impl_draw_content() override;
        void impl_on_close() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

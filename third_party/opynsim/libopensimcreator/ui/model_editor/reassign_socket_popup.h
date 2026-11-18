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
    class ReassignSocketPopup final : public Popup {
    public:
        explicit ReassignSocketPopup(
            Widget* parent,
            std::string_view popup_name,
            std::shared_ptr<opyn::ModelStatePair>,
            std::string_view component_abs_path,
            std::string_view socket_name);

    private:
        void impl_draw_content() override;
        void impl_on_close() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

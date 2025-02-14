#pragma once

#include <liboscar/UI/Popups/Popup.h>

#include <memory>
#include <string_view>

namespace osc { class ParamBlock; }

namespace osc
{
    // popup that edits a parameter block in-place
    class ParamBlockEditorPopup final : public Popup {
    public:
        explicit ParamBlockEditorPopup(
            Widget* parent,
            std::string_view popupName,
            ParamBlock*
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

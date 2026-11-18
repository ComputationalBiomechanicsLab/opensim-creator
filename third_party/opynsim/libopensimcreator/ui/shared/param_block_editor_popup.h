#pragma once

#include <liboscar/ui/popups/popup.h>

#include <memory>
#include <string_view>

namespace osc
{
    class ParamBlock;

    // popup that edits a parameter block in-place
    class ParamBlockEditorPopup final : public Popup {
    public:
        explicit ParamBlockEditorPopup(Widget* parent, std::string_view popup_name, ParamBlock*);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

#pragma once

#include <liboscar/platform/widget.h>

#include <memory>
#include <string_view>

namespace osc
{
    class MwUiSharedState;

    /// Represents the mesh warper UI's top toolbar.
    class MwUiToolbar final : public Widget {
    public:
        explicit MwUiToolbar(
            Widget* parent, std::string_view label, std::shared_ptr<MwUiSharedState>);

    private:
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

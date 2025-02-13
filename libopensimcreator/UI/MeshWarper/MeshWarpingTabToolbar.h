#pragma once

#include <liboscar/Platform/Widget.h>

#include <memory>
#include <string_view>

namespace osc { class MeshWarpingTabSharedState; }

namespace osc
{
    // the top toolbar (contains icons for new, save, open, undo, redo, etc.)
    class MeshWarpingTabToolbar final : public Widget {
    public:
        explicit MeshWarpingTabToolbar(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<MeshWarpingTabSharedState>
        );

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

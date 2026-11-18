#pragma once

#include <liboscar/ui/popups/popup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim
{
    class GeometryPath;
}

namespace opyn
{
    class ComponentAccessor;
}

namespace osc
{

    class GeometryPathEditorPopup final : public Popup {
    public:
        explicit GeometryPathEditorPopup(
            Widget* parent,
            std::string_view popup_name,
            std::shared_ptr<const opyn::ComponentAccessor> target_component,
            std::function<const OpenSim::GeometryPath*()> geometry_path_getter,
            std::function<void(const OpenSim::GeometryPath&)> on_local_copy_edited);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

#pragma once

#include <liboscar/ui/popups/popup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class GeometryPath; }
namespace opyn { class ComponentAccessor; }

namespace osc
{

    class GeometryPathEditorPopup final : public Popup {
    public:
        explicit GeometryPathEditorPopup(
            Widget* parent_,
            std::string_view popupName_,
            std::shared_ptr<const opyn::ComponentAccessor> targetComponent_,
            std::function<const OpenSim::GeometryPath*()> geometryPathGetter_,
            std::function<void(const OpenSim::GeometryPath&)> onLocalCopyEdited_
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

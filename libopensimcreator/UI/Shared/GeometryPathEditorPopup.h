#pragma once

#include <liboscar/UI/Popups/Popup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class GeometryPath; }
namespace osc { class IComponentAccessor; }

namespace osc
{

    class GeometryPathEditorPopup final : public Popup {
    public:
        explicit GeometryPathEditorPopup(
            Widget* parent_,
            std::string_view popupName_,
            std::shared_ptr<const IComponentAccessor> targetComponent_,
            std::function<const OpenSim::GeometryPath*()> geometryPathGetter_,
            std::function<void(const OpenSim::GeometryPath&)> onLocalCopyEdited_
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

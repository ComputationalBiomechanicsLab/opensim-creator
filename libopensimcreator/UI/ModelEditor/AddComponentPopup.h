#pragma once

#include <liboscar/ui/popups/popup.h>
#include <OpenSim/Common/ComponentPath.h>

#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace osc { class ModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class AddComponentPopup final : public Popup {
    public:
        explicit AddComponentPopup(
            Widget* parent,
            std::string_view popupName,
            std::shared_ptr<ModelStatePair>,
            std::unique_ptr<OpenSim::Component> prototype,
            OpenSim::ComponentPath targetComponent = {}
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

#pragma once

#include <liboscar/UI/Popups/Popup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class IModelStatePair; }

namespace osc
{
    // popup for selecting a component of a specified type
    class SelectComponentPopup final : public Popup {
    public:
        explicit SelectComponentPopup(
            Widget* parent,
            std::string_view popupName,
            std::shared_ptr<const IModelStatePair>,
            std::function<void(const OpenSim::ComponentPath&)> onSelection,
            std::function<bool(const OpenSim::Component&)> filter
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

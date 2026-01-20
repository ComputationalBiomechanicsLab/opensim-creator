#pragma once

#include <liboscar/ui/popups/popup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class ModelStatePair; }

namespace osc
{
    class Select1PFPopup final : public Popup {
    public:
        explicit Select1PFPopup(
            Widget* parent,
            std::string_view popupName,
            std::shared_ptr<const ModelStatePair>,
            std::function<void(const OpenSim::ComponentPath&)> onSelection
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

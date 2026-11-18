#pragma once

#include <liboscar/ui/popups/popup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim
{
    class Component;
    class ComponentPath;
}

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    // popup for selecting a component of a specified type
    class SelectComponentPopup final : public Popup {
    public:
        explicit SelectComponentPopup(
            Widget* parent,
            std::string_view popup_name,
            std::shared_ptr<const opyn::ModelStatePair>,
            std::function<void(const OpenSim::ComponentPath&)> on_selection,
            std::function<bool(const OpenSim::Component&)> filter);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

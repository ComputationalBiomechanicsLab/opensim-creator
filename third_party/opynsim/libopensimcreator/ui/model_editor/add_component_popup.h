#pragma once

#include <liboscar/ui/popups/popup.h>
#include <OpenSim/Common/ComponentPath.h>

#include <memory>
#include <string_view>

namespace OpenSim
{
    class Component;
}

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class Widget;

    class AddComponentPopup final : public Popup {
    public:
        explicit AddComponentPopup(
            Widget* parent,
            std::string_view popup_name,
            std::shared_ptr<opyn::ModelStatePair>,
            std::unique_ptr<OpenSim::Component> prototype,
            OpenSim::ComponentPath target_component = {});

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

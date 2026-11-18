#pragma once

#include <liboscar/ui/popups/popup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim
{
    class ComponentPath;
}
namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class Select1PFPopup final : public Popup {
    public:
        explicit Select1PFPopup(
            Widget* parent,
            std::string_view popup_name,
            std::shared_ptr<const opyn::ModelStatePair>,
            std::function<void(const OpenSim::ComponentPath&)> on_selection);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

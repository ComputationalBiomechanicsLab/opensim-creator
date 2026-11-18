#pragma once

#include <liboscar/ui/popups/popup.h>
#include <liboscar/utilities/flags.h>

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
    class Widget;

    enum class ComponentContextMenuFlag : unsigned {
        None = 0,
        NoPlotVsCoordinate = 1 << 0,
    };
    using ComponentContextMenuFlags = Flags<ComponentContextMenuFlag>;

    class ComponentContextMenu final : public Popup {
    public:
        explicit ComponentContextMenu(
            Widget* parent,
            std::string_view popup_name,
            std::shared_ptr<opyn::ModelStatePair>,
            const OpenSim::ComponentPath&,
            ComponentContextMenuFlags = {});

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

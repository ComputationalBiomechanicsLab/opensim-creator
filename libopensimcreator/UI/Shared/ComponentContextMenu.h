#pragma once

#include <liboscar/ui/popups/popup.h>
#include <liboscar/utils/flags.h>

#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    enum class ComponentContextMenuFlag : unsigned {
        None               = 0,
        NoPlotVsCoordinate = 1<<0,
    };
    using ComponentContextMenuFlags = Flags<ComponentContextMenuFlag>;

    class ComponentContextMenu final : public Popup {
    public:
        explicit ComponentContextMenu(
            Widget* parent,
            std::string_view popupName,
            std::shared_ptr<IModelStatePair>,
            const OpenSim::ComponentPath&,
            ComponentContextMenuFlags = {}
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

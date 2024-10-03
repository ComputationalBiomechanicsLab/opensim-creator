#pragma once

#include <oscar/UI/Widgets/IPopup.h>
#include <oscar/Utils/Flags.h>

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

    class ComponentContextMenu final : public IPopup {
    public:
        ComponentContextMenu(
            std::string_view popupName,
            Widget& parent,
            std::shared_ptr<IModelStatePair>,
            const OpenSim::ComponentPath&,
            ComponentContextMenuFlags = {}
        );
        ComponentContextMenu(const ComponentContextMenu&) = delete;
        ComponentContextMenu(ComponentContextMenu&&) noexcept;
        ComponentContextMenu& operator=(const ComponentContextMenu&) = delete;
        ComponentContextMenu& operator=(ComponentContextMenu&&) noexcept;
        ~ComponentContextMenu() noexcept;

    private:
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        bool impl_begin_popup() final;
        void impl_on_draw() final;
        void impl_end_popup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

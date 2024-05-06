#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class IEditorAPI; }
namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ComponentContextMenu final : public IPopup {
    public:
        ComponentContextMenu(
            std::string_view popupName,
            ParentPtr<IMainUIStateAPI> const&,
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            OpenSim::ComponentPath const&
        );
        ComponentContextMenu(ComponentContextMenu const&) = delete;
        ComponentContextMenu(ComponentContextMenu&&) noexcept;
        ComponentContextMenu& operator=(ComponentContextMenu const&) = delete;
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

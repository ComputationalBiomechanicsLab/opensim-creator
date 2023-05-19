#pragma once

#include <oscar/Widgets/Popup.hpp>

#include <memory>
#include <string_view>


namespace OpenSim { class ComponentPath; }
namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ComponentContextMenu final : public Popup {
    public:
        ComponentContextMenu(
            std::string_view popupName,
            std::weak_ptr<MainUIStateAPI>,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            OpenSim::ComponentPath const&
        );
        ComponentContextMenu(ComponentContextMenu const&) = delete;
        ComponentContextMenu(ComponentContextMenu&&) noexcept;
        ComponentContextMenu& operator=(ComponentContextMenu const&) = delete;
        ComponentContextMenu& operator=(ComponentContextMenu&&) noexcept;
        ~ComponentContextMenu() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implDrawPopupContent() final;
        void implEndPopup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
#pragma once

#include <oscar/UI/Widgets/IPopup.hpp>

#include <memory>
#include <string_view>


namespace OpenSim { class ComponentPath; }
namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ComponentContextMenu final : public IPopup {
    public:
        ComponentContextMenu(
            std::string_view popupName,
            ParentPtr<MainUIStateAPI> const&,
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
        void implOnDraw() final;
        void implEndPopup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

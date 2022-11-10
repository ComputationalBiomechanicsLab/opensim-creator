#pragma once

#include "src/Widgets/VirtualPopup.hpp"

#include <memory>
#include <string_view>


namespace OpenSim { class ComponentPath; }
namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ComponentContextMenu final : public VirtualPopup {
    public:
        ComponentContextMenu(
            std::string_view popupName,
            MainUIStateAPI*,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            OpenSim::ComponentPath const&);
        ComponentContextMenu(ComponentContextMenu const&) = delete;
        ComponentContextMenu(ComponentContextMenu&&) noexcept;
        ComponentContextMenu& operator=(ComponentContextMenu const&) = delete;
        ComponentContextMenu& operator=(ComponentContextMenu&&) noexcept;
        ~ComponentContextMenu() noexcept;

        bool isOpen() const override;
        void open() override;
        void close() override;
        bool beginPopup() override;
        void drawPopupContent() override;
        void endPopup() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
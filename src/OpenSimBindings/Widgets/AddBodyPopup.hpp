#pragma once

#include "src/Widgets/Popup.hpp"

#include <memory>
#include <string_view>


namespace osc { class EditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class AddBodyPopup final : public Popup {
    public:
        AddBodyPopup(
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view popupName);
        AddBodyPopup(AddBodyPopup const&) = delete;
        AddBodyPopup(AddBodyPopup&&) noexcept;
        AddBodyPopup& operator=(AddBodyPopup const&) = delete;
        AddBodyPopup& operator=(AddBodyPopup&&) noexcept;
        ~AddBodyPopup() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implDrawPopupContent() final;
        void implEndPopup() final;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

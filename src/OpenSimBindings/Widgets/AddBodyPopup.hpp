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

#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <memory>
#include <string_view>

namespace osc { class IEditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class AddBodyPopup final : public IPopup {
    public:
        AddBodyPopup(
            std::string_view popupName,
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        AddBodyPopup(AddBodyPopup const&) = delete;
        AddBodyPopup(AddBodyPopup&&) noexcept;
        AddBodyPopup& operator=(AddBodyPopup const&) = delete;
        AddBodyPopup& operator=(AddBodyPopup&&) noexcept;
        ~AddBodyPopup() noexcept;

    private:
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        bool impl_begin_popup() final;
        void impl_on_draw() final;
        void impl_end_popup() final;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

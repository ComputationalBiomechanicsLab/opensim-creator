#pragma once

#include <oscar/UI/Popups/IPopup.h>

#include <memory>
#include <string_view>

namespace osc { class ParamBlock; }

namespace osc
{
    // popup that edits a parameter block in-place
    class ParamBlockEditorPopup final : public IPopup {
    public:
        ParamBlockEditorPopup(
            std::string_view popupName,
            ParamBlock*
        );
        ParamBlockEditorPopup(const ParamBlockEditorPopup&) = delete;
        ParamBlockEditorPopup(ParamBlockEditorPopup&&) noexcept;
        ParamBlockEditorPopup& operator=(const ParamBlockEditorPopup&) = delete;
        ParamBlockEditorPopup& operator=(ParamBlockEditorPopup&&) noexcept;
        ~ParamBlockEditorPopup() noexcept;

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

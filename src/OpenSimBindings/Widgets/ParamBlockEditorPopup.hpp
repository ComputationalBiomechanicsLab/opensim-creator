#pragma once

#include "src/Widgets/VirtualPopup.hpp"

#include <string_view>

namespace osc { class ParamBlock; }

namespace osc
{
    // popup that edits a parameter block in-place
    class ParamBlockEditorPopup final : public VirtualPopup {
    public:
        explicit ParamBlockEditorPopup(std::string_view popupName, ParamBlock*);
        ParamBlockEditorPopup(ParamBlockEditorPopup const&) = delete;
        ParamBlockEditorPopup(ParamBlockEditorPopup&&) noexcept;
        ParamBlockEditorPopup& operator=(ParamBlockEditorPopup const&) = delete;
        ParamBlockEditorPopup& operator=(ParamBlockEditorPopup&&) noexcept;
        ~ParamBlockEditorPopup() noexcept;

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

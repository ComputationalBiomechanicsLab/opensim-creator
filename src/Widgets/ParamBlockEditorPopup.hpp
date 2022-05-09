#pragma once

#include <string_view>

namespace osc
{
    class ParamBlock;
}

namespace osc
{
    class ParamBlockEditorPopup final {
    public:
        explicit ParamBlockEditorPopup(std::string_view popupName);
        ParamBlockEditorPopup(ParamBlockEditorPopup const&) = delete;
        ParamBlockEditorPopup(ParamBlockEditorPopup&&) noexcept;
        ParamBlockEditorPopup& operator=(ParamBlockEditorPopup const&) = delete;
        ParamBlockEditorPopup& operator=(ParamBlockEditorPopup&&) noexcept;
        ~ParamBlockEditorPopup() noexcept;

        bool isOpen() const;
        void open();
        void close();

        // - edits block in-place
        // - returns true if an edit was made
        bool draw(ParamBlock&);

        class Impl;
    private:
        Impl* m_Impl;
    };
}

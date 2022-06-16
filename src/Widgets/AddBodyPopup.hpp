#pragma once

#include <memory>
#include <string_view>

namespace osc
{
    class UndoableModelStatePair;
}

namespace osc
{
    class AddBodyPopup final {
    public:
        AddBodyPopup(std::shared_ptr<UndoableModelStatePair>, std::string_view popupName);
        AddBodyPopup(AddBodyPopup const&) = delete;
        AddBodyPopup(AddBodyPopup&&) noexcept;
        AddBodyPopup& operator=(AddBodyPopup const&) = delete;
        AddBodyPopup& operator=(AddBodyPopup&&) noexcept;
        ~AddBodyPopup() noexcept;

        void open();
        void close();
        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}

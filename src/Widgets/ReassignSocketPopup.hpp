#pragma once

#include <memory>
#include <string_view>

namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ReassignSocketPopup final {
    public:
        explicit ReassignSocketPopup(
            std::string_view popupName,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view componentAbsPath,
            std::string_view socketName);
        ReassignSocketPopup(ReassignSocketPopup const&) = delete;
        ReassignSocketPopup(ReassignSocketPopup&&) noexcept;
        ReassignSocketPopup& operator=(ReassignSocketPopup const&) = delete;
        ReassignSocketPopup& operator=(ReassignSocketPopup&&) noexcept;
        ~ReassignSocketPopup() noexcept;

        bool isOpen() const;
        void open();
        void close();
        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}

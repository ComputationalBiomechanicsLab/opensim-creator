#pragma once

#include <oscar/UI/Popups/IPopup.h>

#include <memory>
#include <string_view>

namespace osc { class IModelStatePair; }

namespace osc
{
    class ReassignSocketPopup final : public IPopup {
    public:
        ReassignSocketPopup(
            std::string_view popupName,
            std::shared_ptr<IModelStatePair>,
            std::string_view componentAbsPath,
            std::string_view socketName
        );
        ReassignSocketPopup(const ReassignSocketPopup&) = delete;
        ReassignSocketPopup(ReassignSocketPopup&&) noexcept;
        ReassignSocketPopup& operator=(const ReassignSocketPopup&) = delete;
        ReassignSocketPopup& operator=(ReassignSocketPopup&&) noexcept;
        ~ReassignSocketPopup() noexcept;

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

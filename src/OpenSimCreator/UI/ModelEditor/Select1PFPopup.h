#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class Select1PFPopup final : public IPopup {
    public:
        Select1PFPopup(
            std::string_view popupName,
            std::shared_ptr<const UndoableModelStatePair>,
            std::function<void(const OpenSim::ComponentPath&)> onSelection
        );
        Select1PFPopup(const Select1PFPopup&) = delete;
        Select1PFPopup(Select1PFPopup&&) noexcept;
        Select1PFPopup& operator=(const Select1PFPopup&) = delete;
        Select1PFPopup& operator=(Select1PFPopup&&) noexcept;
        ~Select1PFPopup() noexcept;

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

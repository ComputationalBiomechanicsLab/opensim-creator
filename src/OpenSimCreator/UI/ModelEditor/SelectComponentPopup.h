#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    // popup for selecting a component of a specified type
    class SelectComponentPopup final : public IPopup {
    public:
        SelectComponentPopup(
            std::string_view popupName,
            std::shared_ptr<UndoableModelStatePair const>,
            std::function<void(OpenSim::ComponentPath const&)> onSelection,
            std::function<bool(OpenSim::Component const&)> filter
        );
        SelectComponentPopup(SelectComponentPopup const&) = delete;
        SelectComponentPopup(SelectComponentPopup&&) noexcept;
        SelectComponentPopup& operator=(SelectComponentPopup const&) = delete;
        SelectComponentPopup& operator=(SelectComponentPopup&&) noexcept;
        ~SelectComponentPopup() noexcept;

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

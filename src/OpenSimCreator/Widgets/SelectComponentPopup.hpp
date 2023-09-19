#pragma once

#include <oscar/UI/Widgets/Popup.hpp>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    // popup for selecting a component of a specified type
    class SelectComponentPopup final : public Popup {
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
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implOnDraw() final;
        void implEndPopup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

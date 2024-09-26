#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class IModelStatePair; }

namespace osc
{
    // popup for selecting a component of a specified type
    class SelectComponentPopup final : public IPopup {
    public:
        SelectComponentPopup(
            std::string_view popupName,
            std::shared_ptr<const IModelStatePair>,
            std::function<void(const OpenSim::ComponentPath&)> onSelection,
            std::function<bool(const OpenSim::Component&)> filter
        );
        SelectComponentPopup(const SelectComponentPopup&) = delete;
        SelectComponentPopup(SelectComponentPopup&&) noexcept;
        SelectComponentPopup& operator=(const SelectComponentPopup&) = delete;
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

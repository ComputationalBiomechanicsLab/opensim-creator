#pragma once

#include <liboscar/UI/Popups/IPopup.h>

#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class AddComponentPopup final : public IPopup {
    public:
        AddComponentPopup(
            std::string_view popupName,
            Widget& parent,
            std::shared_ptr<IModelStatePair>,
            std::unique_ptr<OpenSim::Component> prototype
        );
        AddComponentPopup(const AddComponentPopup&) = delete;
        AddComponentPopup(AddComponentPopup&&) noexcept;
        AddComponentPopup& operator=(const AddComponentPopup&) = delete;
        AddComponentPopup& operator=(AddComponentPopup&&) noexcept;
        ~AddComponentPopup() noexcept;

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

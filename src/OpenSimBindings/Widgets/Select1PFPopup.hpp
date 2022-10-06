#pragma once

#include "src/Widgets/Popup.hpp"

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class Select1PFPopup final : public Popup {
    public:
        Select1PFPopup(
            std::string_view popupName,
            std::shared_ptr<UndoableModelStatePair>,
            std::function<void(OpenSim::ComponentPath const&)>);
        Select1PFPopup(Select1PFPopup const&) = delete;
        Select1PFPopup(Select1PFPopup&&) noexcept;
        Select1PFPopup& operator=(Select1PFPopup const&) = delete;
        Select1PFPopup& operator=(Select1PFPopup&&) noexcept;
        ~Select1PFPopup() noexcept;

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

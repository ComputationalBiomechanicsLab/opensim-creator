#pragma once

#include "src/Widgets/Popup.hpp"

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Geometry; }

namespace osc
{
    class SelectGeometryPopup final : public Popup {
    public:
        explicit SelectGeometryPopup(std::string_view popupName, std::function<void(std::unique_ptr<OpenSim::Geometry>)> onSelection);
        SelectGeometryPopup(SelectGeometryPopup const&) = delete;
        SelectGeometryPopup(SelectGeometryPopup&&) noexcept;
        SelectGeometryPopup& operator=(SelectGeometryPopup const&) = delete;
        SelectGeometryPopup& operator=(SelectGeometryPopup&&) noexcept;
        ~SelectGeometryPopup() noexcept;

        bool isOpen() const override;
        void open() override;
        void close() override;
        void draw() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}

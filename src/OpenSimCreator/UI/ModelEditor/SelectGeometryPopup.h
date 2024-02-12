#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Geometry; }

namespace osc
{
    class SelectGeometryPopup final : public IPopup {
    public:
        SelectGeometryPopup(
            std::string_view popupName,
            std::filesystem::path const& geometryDir,
            std::function<void(std::unique_ptr<OpenSim::Geometry>)> onSelection
        );
        SelectGeometryPopup(SelectGeometryPopup const&) = delete;
        SelectGeometryPopup(SelectGeometryPopup&&) noexcept;
        SelectGeometryPopup& operator=(SelectGeometryPopup const&) = delete;
        SelectGeometryPopup& operator=(SelectGeometryPopup&&) noexcept;
        ~SelectGeometryPopup() noexcept;

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

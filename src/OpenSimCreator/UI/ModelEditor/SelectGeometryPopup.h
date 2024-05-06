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

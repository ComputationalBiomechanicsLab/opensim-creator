#pragma once

#include <oscar/UI/Widgets/Popup.hpp>

#include <memory>
#include <string_view>

namespace osc { class MeshImporterSharedState; }

namespace osc
{
    class ImportStationsFromCSVPopup final : public Popup {
    public:
        ImportStationsFromCSVPopup(
            std::string_view,
            std::shared_ptr<MeshImporterSharedState> const&
        );
        ImportStationsFromCSVPopup(ImportStationsFromCSVPopup const&) = delete;
        ImportStationsFromCSVPopup(ImportStationsFromCSVPopup&&) noexcept;
        ImportStationsFromCSVPopup& operator=(ImportStationsFromCSVPopup const&) = delete;
        ImportStationsFromCSVPopup& operator=(ImportStationsFromCSVPopup&&) noexcept;
        ~ImportStationsFromCSVPopup() noexcept;

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

#pragma once

#include <OpenSimCreator/Documents/Landmarks/NamedLandmark.h>

#include <oscar/UI/Widgets/IPopup.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    class ImportStationsFromCSVPopup final : public IPopup {
    public:
        struct ImportedData final {
            std::optional<std::string> maybeLabel;
            std::vector<lm::NamedLandmark> landmarks;
        };

        ImportStationsFromCSVPopup(
            std::string_view,
            std::function<void(ImportedData)> onImport
        );
        ImportStationsFromCSVPopup(const ImportStationsFromCSVPopup&) = delete;
        ImportStationsFromCSVPopup(ImportStationsFromCSVPopup&&) noexcept;
        ImportStationsFromCSVPopup& operator=(const ImportStationsFromCSVPopup&) = delete;
        ImportStationsFromCSVPopup& operator=(ImportStationsFromCSVPopup&&) noexcept;
        ~ImportStationsFromCSVPopup() noexcept;

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

#pragma once

#include <OpenSimCreator/Documents/Landmarks/NamedLandmark.hpp>

#include <oscar/UI/Widgets/IPopup.hpp>

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

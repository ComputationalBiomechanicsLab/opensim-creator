#pragma once

#include <libopensimcreator/Documents/Landmarks/NamedLandmark.h>
#include <libopensimcreator/Documents/Model/IModelStatePair.h>

#include <liboscar/ui/popups/popup.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    class ImportStationsFromCSVPopup final : public Popup {
    public:
        struct ImportedData final {
            std::optional<std::string> maybeLabel;
            std::vector<lm::NamedLandmark> landmarks;
            std::optional<std::string> maybeTargetComponentAbsPath;
        };

        explicit ImportStationsFromCSVPopup(
            Widget* parent,
            std::string_view,
            std::function<void(ImportedData)> onImport,
            std::shared_ptr<const IModelStatePair> maybeAssociatedModel = {}
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

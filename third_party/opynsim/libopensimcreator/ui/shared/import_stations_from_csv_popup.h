#pragma once

#include <libopynsim/documents/landmarks/named_landmark.h>
#include <libopynsim/documents/model/model_state_pair.h>
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
            std::optional<std::string> maybe_label;
            std::vector<opyn::NamedLandmark> landmarks;
            std::optional<std::string> maybe_target_component_abs_path;
        };

        explicit ImportStationsFromCSVPopup(
            Widget* parent,
            std::string_view,
            std::function<void(ImportedData)> on_import,
            std::shared_ptr<const opyn::ModelStatePair> maybe_associated_model = {});

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

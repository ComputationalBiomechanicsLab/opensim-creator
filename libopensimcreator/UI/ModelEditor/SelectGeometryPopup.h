#pragma once

#include <liboscar/ui/popups/Popup.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace OpenSim { class Geometry; }

namespace osc
{
    class SelectGeometryPopup final : public Popup {
    public:
        explicit SelectGeometryPopup(
            Widget* parent,
            std::string_view popupName,
            std::optional<std::filesystem::path> geometryDir,
            std::function<void(std::unique_ptr<OpenSim::Geometry>)> onSelection
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

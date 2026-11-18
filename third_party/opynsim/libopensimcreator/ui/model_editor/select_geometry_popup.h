#pragma once

#include <liboscar/ui/popups/popup.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace OpenSim
{
    class Geometry;
}

namespace osc
{
    class SelectGeometryPopup final : public Popup {
    public:
        explicit SelectGeometryPopup(
            Widget* parent,
            std::string_view popup_name,
            std::optional<std::filesystem::path> geometry_dir,
            std::function<void(std::unique_ptr<OpenSim::Geometry>)> on_selection);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

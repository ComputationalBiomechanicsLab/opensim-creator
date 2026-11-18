#pragma once

#include <libopensimcreator/ui/shared/model_viewer_panel_flags.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_layer.h>

#include <libopynsim/graphics/cached_model_renderer.h>
#include <liboscar/graphics/scene/scene_collision.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/rect.h>
#include <liboscar/utilities/c_string_view.h>
#include <OpenSim/Common/ComponentPath.h>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace OpenSim
{
    class Component;
}

namespace osc
{
    class ModelViewerPanelState final {
    public:
        explicit ModelViewerPanelState(std::string_view panel_name, ModelViewerPanelFlags);

        CStringView get_panel_name() const { return panel_name_; }
        ModelViewerPanelFlags flags() const { return flags_; }

        Rect viewport_ui_rect;
        bool is_left_click_released_without_dragging = false;
        bool is_right_click_released_without_dragging = false;
        std::optional<AABB> maybe_scene_visible_aabb;
        std::optional<SceneCollision> maybe_base_layer_hittest;
        OpenSim::ComponentPath maybe_hovered_component_abs_path;

        std::span<const SceneDecoration> get_drawlist() const
        {
            return cached_model_renderer_.get_drawlist();
        }

        ModelViewerPanelLayer& push_layer(std::unique_ptr<ModelViewerPanelLayer> layer)
        {
            return *layer_queue_.emplace_back(std::move(layer));
        }

        const opyn::CachedModelRenderer& get_renderer() const { return cached_model_renderer_; }

        opyn::CachedModelRenderer& upd_renderer() { return cached_model_renderer_; }

        void flush_layer_queue_to(std::vector<std::unique_ptr<ModelViewerPanelLayer>>& target)
        {
            target.insert(
                target.end(),
                std::make_move_iterator(layer_queue_.begin()),
                std::make_move_iterator(layer_queue_.end()));
            layer_queue_.clear();
        }

    private:
        std::string panel_name_;
        ModelViewerPanelFlags flags_;
        opyn::CachedModelRenderer cached_model_renderer_;
        std::vector<std::unique_ptr<ModelViewerPanelLayer>> layer_queue_;
    };
}

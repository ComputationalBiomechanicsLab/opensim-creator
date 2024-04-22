#pragma once

#include <OpenSimCreator/Graphics/CachedModelRenderer.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelLayer.h>

#include <OpenSim/Common/ComponentPath.h>
#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Utils/CStringView.h>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace OpenSim { class Component; }

namespace osc
{
    class ModelEditorViewerPanelState final {
    public:
        explicit ModelEditorViewerPanelState(std::string_view panelName_);

        CStringView getPanelName() const
        {
            return panel_name_;
        }

        Rect viewportRect{};
        bool isLeftClickReleasedWithoutDragging = false;
        bool isRightClickReleasedWithoutDragging = false;
        std::optional<AABB> maybeSceneAABB;
        std::optional<SceneCollision> maybeBaseLayerHittest;
        OpenSim::ComponentPath maybeHoveredComponentAbsPath;

        std::span<SceneDecoration const> getDrawlist() const
        {
            return m_CachedModelRenderer.getDrawlist();
        }

        ModelEditorViewerPanelLayer& pushLayer(std::unique_ptr<ModelEditorViewerPanelLayer> layer)
        {
            return *m_LayerQueue.emplace_back(std::move(layer));
        }

        CachedModelRenderer const& getRenderer() const
        {
            return m_CachedModelRenderer;
        }

        CachedModelRenderer& updRenderer()
        {
            return m_CachedModelRenderer;
        }

        void flushLayerQueueTo(std::vector<std::unique_ptr<ModelEditorViewerPanelLayer>>& target)
        {
            target.insert(
                target.end(),
                std::make_move_iterator(m_LayerQueue.begin()),
                std::make_move_iterator(m_LayerQueue.end())
            );
            m_LayerQueue.clear();
        }

    private:
        std::string panel_name_;
        CachedModelRenderer m_CachedModelRenderer;
        std::vector<std::unique_ptr<ModelEditorViewerPanelLayer>> m_LayerQueue;
    };
}

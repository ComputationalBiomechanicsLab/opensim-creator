#pragma once

#include <OpenSimCreator/Graphics/CachedModelRenderer.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanelFlags.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanelLayer.h>

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
    class ModelViewerPanelState final {
    public:
        explicit ModelViewerPanelState(
            std::string_view panelName_,
            ModelViewerPanelFlags
        );

        CStringView getPanelName() const
        {
            return panel_name_;
        }

        ModelViewerPanelFlags flags() const
        {
            return m_Flags;
        }

        Rect viewportRect{};
        bool isLeftClickReleasedWithoutDragging = false;
        bool isRightClickReleasedWithoutDragging = false;
        std::optional<AABB> maybeSceneAABB;
        std::optional<SceneCollision> maybeBaseLayerHittest;
        OpenSim::ComponentPath maybeHoveredComponentAbsPath;

        std::span<const SceneDecoration> getDrawlist() const
        {
            return m_CachedModelRenderer.getDrawlist();
        }

        ModelViewerPanelLayer& pushLayer(std::unique_ptr<ModelViewerPanelLayer> layer)
        {
            return *m_LayerQueue.emplace_back(std::move(layer));
        }

        const CachedModelRenderer& getRenderer() const
        {
            return m_CachedModelRenderer;
        }

        CachedModelRenderer& updRenderer()
        {
            return m_CachedModelRenderer;
        }

        void flushLayerQueueTo(std::vector<std::unique_ptr<ModelViewerPanelLayer>>& target)
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
        ModelViewerPanelFlags m_Flags;
        CachedModelRenderer m_CachedModelRenderer;
        std::vector<std::unique_ptr<ModelViewerPanelLayer>> m_LayerQueue;
    };
}

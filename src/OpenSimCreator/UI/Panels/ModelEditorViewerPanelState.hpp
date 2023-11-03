#pragma once

#include <OpenSimCreator/Graphics/CachedModelRenderer.hpp>
#include <OpenSimCreator/UI/Panels/ModelEditorViewerPanelLayer.hpp>

#include <OpenSim/Common/ComponentPath.h>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Scene/SceneCollision.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Utils/CStringView.hpp>

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
            return m_PanelName;
        }

        Rect viewportRect{};
        bool isLeftClickReleasedWithoutDragging = false;
        bool isRightClickReleasedWithoutDragging = false;
        std::optional<AABB> maybeSceneAABB;
        std::optional<SceneCollision> maybeBaseLayerHittest;
        OpenSim::ComponentPath maybeHoveredComponentAbsPath;

        std::span<osc::SceneDecoration const> getDrawlist() const
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
        std::string m_PanelName;
        CachedModelRenderer m_CachedModelRenderer;
        std::vector<std::unique_ptr<ModelEditorViewerPanelLayer>> m_LayerQueue;
    };
}

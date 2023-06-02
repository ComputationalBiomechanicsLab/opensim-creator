#pragma once

#include "OpenSimCreator/Panels/ModelEditorViewerPanelLayer.hpp"

#include <oscar/Graphics/SceneCollision.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Rect.hpp>

#include <nonstd/span.hpp>

#include <optional>
#include <string>

namespace OpenSim { class Component; }

namespace osc
{
	struct ModelEditorViewerPanelState final {
		std::string panelName;
		Rect viewportRect{};
		bool isLeftClickReleasedWithoutDragging = false;
		bool isRightClickReleasedWithoutDragging = false;
		std::optional<AABB> maybeSceneAABB;
		std::optional<SceneCollision> maybeBaseLayerHittest;
		OpenSim::Component const* maybeHoveredComponent = nullptr;

		nonstd::span<osc::SceneDecoration const> getDrawlist() const
		{
			return {};  // TODO
		}

		void pushLayer(std::unique_ptr<ModelEditorViewerPanelLayer>)
		{
			// todo
		}
	};
}
#pragma once

#include <oscar/Graphics/SceneCollision.hpp>
#include <oscar/Maths/Rect.hpp>

#include <optional>

namespace osc
{
	struct ModelEditorViewerPanelState final {
		Rect viewportRect;
		std::optional<SceneCollision> maybeBaseLayerHittest;
	};
}
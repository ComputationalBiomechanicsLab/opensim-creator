#pragma once

#include <cstdint>

namespace osc
{
	using ModelEditorViewerPanelLayerFlags = int32_t;
	enum ModelEditorViewerPanelLayerFlags_ : int32_t {
		ModelEditorViewerPanelLayerFlags_CapturesInputs = 1<<0,
	
		ModelEditorViewerPanelLayerFlags_Default =
			ModelEditorViewerPanelLayerFlags_CapturesInputs
	};
}

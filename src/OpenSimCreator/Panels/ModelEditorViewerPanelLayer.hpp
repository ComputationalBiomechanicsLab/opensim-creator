#pragma once

#include "OpenSimCreator/Panels/ModelEditorViewerPanelLayerFlags.hpp"

#include <oscar/Graphics/Color.hpp>

#include <SDL_events.h>

namespace osc { class ModelEditorViewerPanelParameters; }
namespace osc { struct ModelEditorViewerPanelState; }

namespace osc
{
	class ModelEditorViewerPanelLayer {
	protected:
        ModelEditorViewerPanelLayer() = default;
		ModelEditorViewerPanelLayer(ModelEditorViewerPanelLayer const&) = default;
		ModelEditorViewerPanelLayer(ModelEditorViewerPanelLayer&&) noexcept = default;
		ModelEditorViewerPanelLayer& operator=(ModelEditorViewerPanelLayer const&) = default;
		ModelEditorViewerPanelLayer& operator=(ModelEditorViewerPanelLayer&&) noexcept = default;
	public:
		virtual ~ModelEditorViewerPanelLayer() noexcept = default;

        ModelEditorViewerPanelLayerFlags getFlags() const
        {
            return implGetFlags();
        }

        float getBackgroundAlpha() const
        {
            return implGetBackgroundAlpha();
        }

        void onDraw(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState const& state)
        {
            implOnDraw(params, state);
        }

        bool shouldClose() const
        {
            return implShouldClose();
        }

	private:
        virtual ModelEditorViewerPanelLayerFlags implGetFlags() const
        {
            return ModelEditorViewerPanelLayerFlags_Default;
        }

        virtual float implGetBackgroundAlpha() const
        {
            return 0.0f;
        }

        virtual void implOnDraw(ModelEditorViewerPanelParameters&, ModelEditorViewerPanelState const&) = 0;
        virtual bool implShouldClose() const = 0;
	};
}
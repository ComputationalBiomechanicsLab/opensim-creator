#pragma once

#include "OpenSimCreator/Panels/ModelEditorViewerPanelLayerFlags.hpp"

#include <oscar/Graphics/Color.hpp>

#include <SDL_events.h>

namespace osc { class ModelEditorViewerPanelParameters; }
namespace osc { class ModelEditorViewerPanelState; }

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

        void onNewFrame()
        {
            implOnNewFrame();
        }

        bool handleMouseInputs(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState& state)
        {
            return implHandleMouseInputs(params, state);
        }

        bool handleKeyboardInputs(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState& state)
        {
            return implHandleKeyboardInputs(params, state);
        }

        void onDraw(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState& state)
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

        virtual void implOnNewFrame()
        {
        }

        virtual bool implHandleMouseInputs(
            ModelEditorViewerPanelParameters&,
            ModelEditorViewerPanelState&)
        {
            return false;
        }

        virtual bool implHandleKeyboardInputs(
            ModelEditorViewerPanelParameters&,
            ModelEditorViewerPanelState&)
        {
            return false;
        }

        virtual void implOnDraw(
            ModelEditorViewerPanelParameters&,
            ModelEditorViewerPanelState&) = 0;

        virtual bool implShouldClose() const = 0;
    };
}

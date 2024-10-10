#pragma once

#include <OpenSimCreator/UI/Shared/ModelViewerPanelLayerFlags.h>

namespace osc { class ModelViewerPanelParameters; }
namespace osc { class ModelViewerPanelState; }

namespace osc
{
    class ModelViewerPanelLayer {
    protected:
        ModelViewerPanelLayer() = default;
        ModelViewerPanelLayer(const ModelViewerPanelLayer&) = default;
        ModelViewerPanelLayer(ModelViewerPanelLayer&&) noexcept = default;
        ModelViewerPanelLayer& operator=(const ModelViewerPanelLayer&) = default;
        ModelViewerPanelLayer& operator=(ModelViewerPanelLayer&&) noexcept = default;
    public:
        virtual ~ModelViewerPanelLayer() noexcept = default;

        ModelViewerPanelLayerFlags getFlags() const
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
            ModelViewerPanelParameters& params,
            ModelViewerPanelState& state)
        {
            return implHandleMouseInputs(params, state);
        }

        bool handleKeyboardInputs(
            ModelViewerPanelParameters& params,
            ModelViewerPanelState& state)
        {
            return implHandleKeyboardInputs(params, state);
        }

        void onDraw(
            ModelViewerPanelParameters& params,
            ModelViewerPanelState& state)
        {
            implOnDraw(params, state);
        }

        bool shouldClose() const
        {
            return implShouldClose();
        }

    private:
        virtual ModelViewerPanelLayerFlags implGetFlags() const
        {
            return ModelViewerPanelLayerFlags::Default;
        }

        virtual float implGetBackgroundAlpha() const
        {
            return 0.0f;
        }

        virtual void implOnNewFrame()
        {
        }

        virtual bool implHandleMouseInputs(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&)
        {
            return false;
        }

        virtual bool implHandleKeyboardInputs(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&)
        {
            return false;
        }

        virtual void implOnDraw(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&) = 0;

        virtual bool implShouldClose() const = 0;
    };
}

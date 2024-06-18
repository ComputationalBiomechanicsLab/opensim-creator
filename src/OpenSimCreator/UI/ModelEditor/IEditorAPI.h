#pragma once

#include <OpenSimCreator/UI/IPopupAPI.h>

#include <memory>

namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class Muscle; }
namespace osc { class ModelMusclePlotPanel; }
namespace osc { class PanelManager; }

namespace osc
{
    class IEditorAPI : public IPopupAPI {
    protected:
        IEditorAPI() = default;
        IEditorAPI(const IEditorAPI&) = default;
        IEditorAPI(IEditorAPI&&) noexcept = default;
        IEditorAPI& operator=(const IEditorAPI&) = default;
        IEditorAPI& operator=(IEditorAPI&&) noexcept = default;
    public:
        virtual ~IEditorAPI() noexcept = default;

        void pushComponentContextMenuPopup(const OpenSim::ComponentPath& p)
        {
            implPushComponentContextMenuPopup(p);
        }

        void addMusclePlot(const OpenSim::Coordinate& coord, const OpenSim::Muscle& muscle)
        {
            implAddMusclePlot(coord, muscle);
        }

        std::shared_ptr<PanelManager> getPanelManager()
        {
            return implGetPanelManager();
        }

    private:
        virtual void implPushComponentContextMenuPopup(const OpenSim::ComponentPath&) = 0;
        virtual void implAddMusclePlot(const OpenSim::Coordinate&, const OpenSim::Muscle&) = 0;
        virtual std::shared_ptr<PanelManager> implGetPanelManager() = 0;
    };
}

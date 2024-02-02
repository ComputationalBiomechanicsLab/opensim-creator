#pragma once

#include <OpenSimCreator/UI/IPopupAPI.hpp>

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
        IEditorAPI(IEditorAPI const&) = default;
        IEditorAPI(IEditorAPI&&) noexcept = default;
        IEditorAPI& operator=(IEditorAPI const&) = default;
        IEditorAPI& operator=(IEditorAPI&&) noexcept = default;
    public:
        virtual ~IEditorAPI() noexcept = default;

        void pushComponentContextMenuPopup(OpenSim::ComponentPath const& p)
        {
            implPushComponentContextMenuPopup(p);
        }

        void addMusclePlot(OpenSim::Coordinate const& coord, OpenSim::Muscle const& muscle)
        {
            implAddMusclePlot(coord, muscle);
        }

        std::shared_ptr<PanelManager> getPanelManager()
        {
            return implGetPanelManager();
        }

    private:
        virtual void implPushComponentContextMenuPopup(OpenSim::ComponentPath const&) = 0;
        virtual void implAddMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&) = 0;
        virtual std::shared_ptr<PanelManager> implGetPanelManager() = 0;
    };
}

#pragma once

#include "src/Widgets/Popup.hpp"

#include <memory>
#include <utility>

namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class Muscle; }

namespace osc
{
    class EditorAPI {
    protected:
        EditorAPI() = default;
        EditorAPI(EditorAPI const&) = default;
        EditorAPI(EditorAPI&&) noexcept = default;
        EditorAPI& operator=(EditorAPI const&) = default;
        EditorAPI& operator=(EditorAPI&&) noexcept = default;
    public:
        virtual ~EditorAPI() noexcept = default;

        void pushComponentContextMenuPopup(OpenSim::ComponentPath const& p)
        {
            implPushComponentContextMenuPopup(p);
        }

        void pushPopup(std::unique_ptr<Popup> p)
        {
            implPushPopup(std::move(p));
        }

        void addMusclePlot(OpenSim::Coordinate const& coord, OpenSim::Muscle const& muscle)
        {
            implAddMusclePlot(coord, muscle);
        }

    private:
        virtual void implPushComponentContextMenuPopup(OpenSim::ComponentPath const&) = 0;
        virtual void implPushPopup(std::unique_ptr<Popup>) = 0;
        virtual void implAddMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&) = 0;
    };
}

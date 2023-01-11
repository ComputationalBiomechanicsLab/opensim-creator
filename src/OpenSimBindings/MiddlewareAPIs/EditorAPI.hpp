#pragma once

#include "src/Widgets/Popup.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class Muscle; }
namespace osc { class ModelMusclePlotPanel; }

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

        size_t getNumMusclePlots()
        {
            return implGetNumMusclePlots();
        }

        ModelMusclePlotPanel const& getMusclePlot(ptrdiff_t i)
        {
            return implGetMusclePlot(i);
        }

        void addEmptyMusclePlot()
        {
            implAddEmptyMusclePlot();
        }

        void addMusclePlot(OpenSim::Coordinate const& coord, OpenSim::Muscle const& muscle)
        {
            implAddMusclePlot(coord, muscle);
        }

        void deleteMusclePlot(ptrdiff_t i)
        {
            implDeleteMusclePlot(i);
        }

        void addVisualizer()
        {
            implAddVisualizer();
        }

        size_t getNumModelVisualizers()
        {
            return implGetNumModelVisualizers();
        }

        std::string getModelVisualizerName(ptrdiff_t i)
        {
            return implGetModelVisualizerName(i);
        }

        void deleteVisualizer(ptrdiff_t i)
        {
            implDeleteVisualizer(i);
        }

    private:
        virtual void implPushComponentContextMenuPopup(OpenSim::ComponentPath const&) = 0;
        virtual void implPushPopup(std::unique_ptr<Popup>) = 0;
        virtual size_t implGetNumMusclePlots() = 0;
        virtual ModelMusclePlotPanel const& implGetMusclePlot(ptrdiff_t) = 0;
        virtual void implAddEmptyMusclePlot() = 0;
        virtual void implAddMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&) = 0;
        virtual void implDeleteMusclePlot(ptrdiff_t) = 0;
        virtual void implAddVisualizer() = 0;
        virtual size_t implGetNumModelVisualizers() = 0;
        virtual std::string implGetModelVisualizerName(ptrdiff_t i) = 0;
        virtual void implDeleteVisualizer(ptrdiff_t) = 0;
    };
}

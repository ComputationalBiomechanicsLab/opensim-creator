#pragma once

namespace OpenSim { class Coordinate; }
namespace OpenSim { class Muscle; }
namespace osc { class Popup; }

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

        virtual void pushPopup(std::unique_ptr<Popup>) = 0;
        virtual void addMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&) = 0;
    };
}